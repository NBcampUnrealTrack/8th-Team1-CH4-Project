#include "SpartaArcadeRoomGenerator.h"

namespace
{
    struct FShape { int32 Count; FIntPoint Cells[6]; };

    // 슬롯 병합 모양 — 모두 (0,0)(앵커=현재 슬롯) 포함. 작은 정사각형(1칸) 없음.
    //   1x2/2x1, 1x3/3x1 = 직사각형 / 2x2 = 중간 정사각형 / 2x3/3x2 = 큰 직사각형 / ㄴ자(두 팔 뚜렷)
    static const FShape GShapes[] = {
        { 2, { FIntPoint(0,0), FIntPoint(1,0) } },                                                   // 1x2
        { 2, { FIntPoint(0,0), FIntPoint(0,1) } },                                                   // 1x2 세로
        { 3, { FIntPoint(0,0), FIntPoint(1,0), FIntPoint(2,0) } },                                   // 1x3 (긴)
        { 3, { FIntPoint(0,0), FIntPoint(0,1), FIntPoint(0,2) } },                                   // 1x3 세로
        { 4, { FIntPoint(0,0), FIntPoint(1,0), FIntPoint(0,1), FIntPoint(1,1) } },                   // 2x2
        { 6, { FIntPoint(0,0), FIntPoint(1,0), FIntPoint(2,0), FIntPoint(0,1), FIntPoint(1,1), FIntPoint(2,1) } }, // 2x3
        { 6, { FIntPoint(0,0), FIntPoint(0,1), FIntPoint(0,2), FIntPoint(1,0), FIntPoint(1,1), FIntPoint(1,2) } }, // 3x2
        { 5, { FIntPoint(0,0), FIntPoint(0,1), FIntPoint(0,2), FIntPoint(1,2), FIntPoint(2,2) } },   // ㄴ
        { 5, { FIntPoint(0,0), FIntPoint(0,1), FIntPoint(0,2), FIntPoint(1,0), FIntPoint(2,0) } },   // Γ
        { 5, { FIntPoint(0,0), FIntPoint(1,0), FIntPoint(2,0), FIntPoint(2,1), FIntPoint(2,2) } },   // ⌐(회전)
        { 4, { FIntPoint(0,0), FIntPoint(0,1), FIntPoint(0,2), FIntPoint(1,2) } },                   // 작은 ㄴ
        { 4, { FIntPoint(0,0), FIntPoint(1,0), FIntPoint(2,0), FIntPoint(0,1) } },                   // 작은 Γ
        { 4, { FIntPoint(0,0), FIntPoint(1,0), FIntPoint(2,0), FIntPoint(2,1) } },                   // 작은 ⌐
    };

    // 간단한 union-find — 방을 스패닝 트리로 잇는 데 사용.
    struct FDSU
    {
        TArray<int32> Parent;
        void Init(int32 N) { Parent.SetNum(N); for (int32 i = 0; i < N; ++i) Parent[i] = i; }
        int32 Find(int32 x) { while (Parent[x] != x) { Parent[x] = Parent[Parent[x]]; x = Parent[x]; } return x; }
        bool Union(int32 a, int32 b) { a = Find(a); b = Find(b); if (a == b) return false; Parent[a] = b; return true; }
    };

    struct FRoomGenImpl
    {
        FSpartaArcadeMapGrid& Grid;
        FRandomStream Rng;
        FSpartaArcadeRoomGenParams P;

        int32 SW = 0, SH = 0;
        TArray<int32> SlotRoom;   // 슬롯 idx → 방 id (-1 미할당, -2 void, >=0 방)
        TArray<int32> CellRoom;   // 셀 idx → 방 id (-1 없음). 문 탐지용(임시)
        int32 RoomCount = 0;
        TArray<FIntPoint> Spawns; // 4모서리 스폰 칸
        TArray<FIntPoint> ObstacleSpawns; // 이동 장애물 스폰 칸(중앙·스폰 제외)
        FIntPoint CenterMin = FIntPoint(0, 0); // 중앙 아레나 bbox(자기장 최종 지대)
        FIntPoint CenterMax = FIntPoint(0, 0);

        FRoomGenImpl(FSpartaArcadeMapGrid& InGrid, int32 Seed, const FSpartaArcadeRoomGenParams& InP)
            : Grid(InGrid), Rng(Seed), P(InP) {
        }

        int32 NumSlots() const { return P.SectorCols * P.SectorRows; }
        int32 Sidx(int32 sx, int32 sy) const { return sy * P.SectorCols + sx; }
        bool SInB(int32 sx, int32 sy) const { return sx >= 0 && sx < P.SectorCols && sy >= 0 && sy < P.SectorRows; }
        int32 SlotX0(int32 sx) const { return P.Gap + sx * (SW + P.Gap); }
        int32 SlotY0(int32 sy) const { return P.Gap + sy * (SH + P.Gap); }

        int32 CellRoomAt(int32 x, int32 y) const
        {
            if (!Grid.IsInside(x, y)) return -1;
            return CellRoom[Grid.IndexOf(x, y)];
        }

        void Run()
        {
            SW = (P.Width - P.Gap * (P.SectorCols + 1)) / P.SectorCols;
            SH = (P.Height - P.Gap * (P.SectorRows + 1)) / P.SectorRows;
            Grid.Init(P.Width, P.Height, ESpartaArcadeTileType::Void);   // 전부 빈 공간으로 시작
            if (SW < 1 || SH < 1) return;
            CellRoom.Init(-1, P.Width * P.Height);

            AssignRooms();   // 중앙 고정 + 전부 채우고 작은 방부터 연결-안전하게 void로 제거
            FillFloors();    // 방 슬롯 + 같은 방 슬롯 사이 갭 → 바닥(Empty)
            CarveDoors();    // 뚫린 길(스패닝 트리) + 드문 부술 수 있는 벽 + 나머지는 부술 수 없는 벽
            BuildWalls();    // 바닥/박스에 면한 void → 고정벽(방 외곽 폐합)
            PopulateInteriors(); // 방 내부: 기둥(FixedWall)+박스(DestructibleBox), 중앙은 랜덤 기하 패턴
            PlaceSpawns(P.SafeRadius); // 4모서리 스폰 + 주변 안전구역 비우기
            PlaceVariants(P.VariantCoverage, P.bVariantsInCenter, P.SafeRadius); // 얼음/물/덤불 패치
            PickObstacleSpawns(P.NumObstacles, P.SafeRadius); // 이동 장애물 스폰 위치(액터는 GameMode가 스폰)
            ComputeCenterBounds(); // 중앙 아레나 bbox(자기장 최종 지대)
        }

        bool ShapeFits(int32 sx, int32 sy, const FShape& Sh) const
        {
            for (int32 c = 0; c < Sh.Count; ++c)
            {
                const int32 nx = sx + Sh.Cells[c].X, ny = sy + Sh.Cells[c].Y;
                if (!SInB(nx, ny)) return false;
                if (SlotRoom[Sidx(nx, ny)] != -1) return false;
            }
            return true;
        }

        // Excl을 void로 친다고 가정했을 때, 남은 방 슬롯들이 한 덩어리로 연결돼 있는지(상하좌우).
        bool SlotsConnectedExcept(const TSet<int32>& Excl) const
        {
            const int32 NS = NumSlots();
            int32 Start = -1, Live = 0;
            for (int32 si = 0; si < NS; ++si)
                if (SlotRoom[si] >= 0 && !Excl.Contains(si)) { ++Live; if (Start < 0) Start = si; }
            if (Live == 0) return true;

            const int32 NB[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
            TSet<int32> Seen; Seen.Add(Start);
            TArray<int32> Stack; Stack.Push(Start);
            int32 Reached = 0;
            while (Stack.Num() > 0)
            {
                const int32 si = Stack.Pop();
                ++Reached;
                const int32 sx = si % P.SectorCols, sy = si / P.SectorCols;
                for (int32 d = 0; d < 4; ++d)
                {
                    const int32 nx = sx + NB[d][0], ny = sy + NB[d][1];
                    if (!SInB(nx, ny)) continue;
                    const int32 nb = Sidx(nx, ny);
                    if (Excl.Contains(nb) || SlotRoom[nb] < 0 || Seen.Contains(nb)) continue;
                    Seen.Add(nb); Stack.Push(nb);
                }
            }
            return Reached == Live;
        }

        void AssignRooms()
        {
            const int32 NS = NumSlots();
            const int32 NumShapes = UE_ARRAY_COUNT(GShapes);
            SlotRoom.Init(-1, NS);

            // 중앙 정사각형(cs x cs) = room 0
            const int32 cs = FMath::Clamp(P.CenterSlots, 1, FMath::Min(P.SectorCols, P.SectorRows));
            const int32 ccx = P.SectorCols / 2 - cs / 2;
            const int32 ccy = P.SectorRows / 2 - cs / 2;

            TArray<TArray<int32>> RoomSlots;   // 방 id → 슬롯 목록
            RoomSlots.Add(TArray<int32>());    // room 0 = center
            for (int32 oy = 0; oy < cs; ++oy)
                for (int32 ox = 0; ox < cs; ++ox)
                {
                    const int32 si = Sidx(ccx + ox, ccy + oy);
                    SlotRoom[si] = 0; RoomSlots[0].Add(si);
                }
            RoomCount = 1;

            // 1) 모든 슬롯을 방으로 채움(합치면 직사각형/ㄴ자, 안 합쳐지면 단일). 일단 꽉 채워 연결 확보.
            TArray<int32> Order;
            Order.SetNum(NS);
            for (int32 i = 0; i < NS; ++i) Order[i] = i;
            for (int32 i = NS - 1; i > 0; --i) { const int32 j = Rng.RandRange(0, i); Order.Swap(i, j); }

            for (int32 idx : Order)
            {
                if (SlotRoom[idx] != -1) continue;
                const int32 sx = idx % P.SectorCols, sy = idx / P.SectorCols;
                const int32 Rid = RoomCount;

                TArray<int32> PlacedSlots;
                if (Rng.FRand() < P.MergeChance)
                {
                    int32 Fitting[16]; int32 FitN = 0;
                    for (int32 s = 0; s < NumShapes; ++s)
                        if (ShapeFits(sx, sy, GShapes[s])) Fitting[FitN++] = s;
                    if (FitN > 0)
                    {
                        const FShape& Sh = GShapes[Fitting[Rng.RandRange(0, FitN - 1)]];
                        for (int32 c = 0; c < Sh.Count; ++c)
                            PlacedSlots.Add(Sidx(sx + Sh.Cells[c].X, sy + Sh.Cells[c].Y));
                    }
                }
                if (PlacedSlots.Num() == 0) PlacedSlots.Add(idx);   // 단일 슬롯(작은 정사각형) → 우선 제거 대상

                for (int32 si : PlacedSlots) SlotRoom[si] = Rid;
                RoomSlots.Add(PlacedSlots);
                ++RoomCount;
            }

            // 2) 작은 방부터 "지워도 나머지 방이 연결된 채 남는 것만" void로 제거(작은 정사각형 제거 + 빈 공간↑).
            struct FCandKey { int32 Room; int32 Key; };
            // 4개 모서리 슬롯(스폰 위치)에는 항상 방이 존재하도록 제거 대상에서 모서리 방 제외
            const int32 CornerSlotsIdx[4] = {
                Sidx(0, 0),
                Sidx(P.SectorCols - 1, 0),
                Sidx(0, P.SectorRows - 1),
                Sidx(P.SectorCols - 1, P.SectorRows - 1)
            };

            TArray<FCandKey> CK;
            for (int32 r = 1; r < RoomCount; ++r)   // center(0) 제외
            {
                if (RoomSlots[r].Num() > 0)
                {
                    bool bContainsCorner = false;
                    for (int32 csIdx : CornerSlotsIdx)
                    {
                        if (RoomSlots[r].Contains(csIdx))
                        {
                            bContainsCorner = true;
                            break;
                        }
                    }
                    if (!bContainsCorner)
                    {
                        CK.Add({ r, RoomSlots[r].Num() * 1000 + Rng.RandRange(0, 999) });
                    }
                }
            }
            CK.Sort([](const FCandKey& A, const FCandKey& B) { return A.Key < B.Key; });

            TSet<int32> Voided;
            int32 Removed = 0;
            for (const FCandKey& C : CK)
            {
                if (Removed >= P.VoidSlots) break;
                TSet<int32> Test = Voided;
                for (int32 si : RoomSlots[C.Room]) Test.Add(si);
                if (SlotsConnectedExcept(Test)) { Voided = Test; ++Removed; }
            }
            for (int32 si : Voided) SlotRoom[si] = -2;
        }

        void StampRange(int32 x0, int32 y0, int32 x1, int32 y1, int32 r)  // 포함 범위
        {
            for (int32 y = y0; y <= y1; ++y)
                for (int32 x = x0; x <= x1; ++x)
                {
                    Grid.SetTile(x, y, ESpartaArcadeTileType::Empty);
                    if (Grid.IsInside(x, y)) CellRoom[Grid.IndexOf(x, y)] = r;
                }
        }

        void FillFloors()
        {
            for (int32 sy = 0; sy < P.SectorRows; ++sy)
                for (int32 sx = 0; sx < P.SectorCols; ++sx)
                {
                    const int32 r = SlotRoom[Sidx(sx, sy)];
                    if (r < 0) continue;
                    const int32 x0 = SlotX0(sx), y0 = SlotY0(sy);
                    StampRange(x0, y0, x0 + SW - 1, y0 + SH - 1, r);
                }
            // 같은 방인 인접 슬롯 사이 갭 메우기(방을 하나로)
            for (int32 sy = 0; sy < P.SectorRows; ++sy)
                for (int32 sx = 0; sx < P.SectorCols; ++sx)
                {
                    const int32 r = SlotRoom[Sidx(sx, sy)];
                    if (r < 0) continue;
                    const int32 x0 = SlotX0(sx), y0 = SlotY0(sy);
                    if (sx + 1 < P.SectorCols && SlotRoom[Sidx(sx + 1, sy)] == r)
                        StampRange(x0 + SW, y0, SlotX0(sx + 1) - 1, y0 + SH - 1, r);  // 가로 갭
                    if (sy + 1 < P.SectorRows && SlotRoom[Sidx(sx, sy + 1)] == r)
                        StampRange(x0, y0 + SH, x0 + SW - 1, SlotY0(sy + 1) - 1, r);  // 세로 갭
                }
        }

        void CarveDoors()
        {
            // pair(작은id,큰id) → 경계 칸(void인데 한 축의 양쪽이 서로 다른 방 바닥)
            TMap<TPair<int32, int32>, TArray<FIntPoint>> PairCells;
            for (int32 y = 0; y < P.Height; ++y)
                for (int32 x = 0; x < P.Width; ++x)
                {
                    if (Grid.GetTile(x, y) != ESpartaArcadeTileType::Void) continue;
                    int32 ra = -1, rb = -1;
                    if (Grid.GetTile(x - 1, y) == ESpartaArcadeTileType::Empty &&
                        Grid.GetTile(x + 1, y) == ESpartaArcadeTileType::Empty)
                    {
                        ra = CellRoomAt(x - 1, y); rb = CellRoomAt(x + 1, y);
                    }
                    else if (Grid.GetTile(x, y - 1) == ESpartaArcadeTileType::Empty &&
                        Grid.GetTile(x, y + 1) == ESpartaArcadeTileType::Empty)
                    {
                        ra = CellRoomAt(x, y - 1); rb = CellRoomAt(x, y + 1);
                    }

                    if (ra >= 0 && rb >= 0 && ra != rb)
                        PairCells.FindOrAdd(TPair<int32, int32>(FMath::Min(ra, rb), FMath::Max(ra, rb)))
                        .Add(FIntPoint(x, y));
                }

            // 스패닝 트리(union-find)로 "뚫린 길"은 연결에 필요한 최소한 + 약간의 순환로만.
            TArray<TPair<int32, int32>> Pairs;
            PairCells.GetKeys(Pairs);
            for (int32 i = Pairs.Num() - 1; i > 0; --i) { const int32 j = Rng.RandRange(0, i); Pairs.Swap(i, j); }

            FDSU Dsu; Dsu.Init(RoomCount);
            TSet<TPair<int32, int32>> OpenPairs;
            for (const TPair<int32, int32>& Pr : Pairs)
            {
                if (Dsu.Union(Pr.Key, Pr.Value)) OpenPairs.Add(Pr);              // 트리 간선 = 뚫린 길
                else if (Rng.FRand() < P.ExtraOpenChance) OpenPairs.Add(Pr);    // 약간의 순환로
            }

            // 경계마다: 뚫린 길이면 한 칸 개방 / 아니면 일부만 부술 수 있는 벽 / 나머지는 Void(→고정벽).
            for (auto& KV : PairCells)
            {
                TArray<FIntPoint>& Cells = KV.Value;
                if (Cells.Num() == 0) continue;
                const FIntPoint Mid = Cells[Cells.Num() / 2];   // 통로는 가운데 한 칸
                if (OpenPairs.Contains(KV.Key))
                    Grid.SetTile(Mid.X, Mid.Y, ESpartaArcadeTileType::Empty);            // 뚫린 길
                else if (Rng.FRand() < P.BreakableWallChance)
                    Grid.SetTile(Mid.X, Mid.Y, ESpartaArcadeTileType::DestructibleBox);  // 부술 수 있는 벽
                // else: 전부 Void 유지 → BuildWalls가 부술 수 없는 벽으로
            }
        }

        void BuildWalls()
        {
            // 바닥/박스에 면한 void → 고정벽(부술 수 없는 벽). 나머지 void(방 안 빈 구멍 등)는 그대로.
            const int32 NB[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
            TArray<FIntPoint> ToWall;
            for (int32 y = 0; y < P.Height; ++y)
                for (int32 x = 0; x < P.Width; ++x)
                {
                    if (Grid.GetTile(x, y) != ESpartaArcadeTileType::Void) continue;
                    bool bNear = false;
                    for (int32 d = 0; d < 4; ++d)
                    {
                        const ESpartaArcadeTileType T = Grid.GetTile(x + NB[d][0], y + NB[d][1]);
                        if (T == ESpartaArcadeTileType::Empty || T == ESpartaArcadeTileType::DestructibleBox)
                        {
                            bNear = true; break;
                        }
                    }
                    if (bNear) ToWall.Add(FIntPoint(x, y));
                }
            for (const FIntPoint& C : ToWall)
                Grid.SetTile(C.X, C.Y, ESpartaArcadeTileType::FixedWall);
        }

        // 중앙 아레나 결정적 기하 패턴. 참 중심 기준 대칭(짝수/홀수 칸 모두 가운데 정렬).
        // 반환: 0=열린 바닥, 1=기둥(FixedWall), 2=박스(DestructibleBox).
        int32 CenterPatternTile(int32 idx, int32 lx, int32 ly, int32 w, int32 h) const
        {
            const float cx = (w - 1) * 0.5f;
            const float cy = (h - 1) * 0.5f;
            const float adx = FMath::Abs(lx - cx);   // 중심에서 거리(짝수 칸이면 0.5,1.5.. / 홀수면 0,1,2..)
            const float ady = FMath::Abs(ly - cy);
            const int32 rx = FMath::FloorToInt(adx + 0.5f);   // 중심 기준 링 번호(대칭)
            const int32 ry = FMath::FloorToInt(ady + 0.5f);
            auto Ev = [](int32 v) { return v % 2 == 0; };
            if (idx == 0) // checker — 중심 정렬 격자
            {
                if (Ev(rx) && Ev(ry)) return 1;
                if (!Ev(rx) && !Ev(ry)) return 2;
                return 0;
            }
            if (idx == 1) // plus — 중심 십자 통로 + 격자
            {
                if (adx <= 1.0f || ady <= 1.0f) return 0;
                if (Ev(rx) && Ev(ry)) return 1;
                if (!Ev(rx) && !Ev(ry)) return 2;
                return 0;
            }
            if (idx == 2) // frames — 동심 박스 액자
            {
                const int32 r = FMath::FloorToInt(FMath::Max(adx, ady) + 0.5f);
                if (r == 0) return 1;
                if (r % 2 == 1) return 2;
                if (Ev(rx) && Ev(ry)) return 1;
                return 0;
            }
            // idx == 3: clusters — 중심 기준 규칙적 십자 덩어리
            const int32 mx = rx % 4, my = ry % 4;
            if (mx == 2 && my == 2) return 1;
            if ((mx == 2 && (my == 1 || my == 3)) || (my == 2 && (mx == 1 || mx == 3))) return 2;
            return 0;
        }

        void PopulateInteriors()
        {
            const int32 W = P.Width, H = P.Height;
            const int32 B = FMath::Max(1, P.InteriorBlock);

            // 열린 문 = Empty인데 CellRoom 미할당(<0). 그 주변을 비워 입구 막힘 방지.
            TSet<int32> ClearCells;
            const int32 R = FMath::Max(0, P.DoorClearRadius);
            for (int32 y = 0; y < H; ++y)
                for (int32 x = 0; x < W; ++x)
                    if (Grid.GetTile(x, y) == ESpartaArcadeTileType::Empty && CellRoomAt(x, y) < 0)
                        for (int32 dy = -R; dy <= R; ++dy)
                            for (int32 dx = -R; dx <= R; ++dx)
                                if (Grid.IsInside(x + dx, y + dy))
                                    ClearCells.Add(Grid.IndexOf(x + dx, y + dy));

            // 일반 방 바닥 = Empty & CellRoom>0(중앙 0·문 -1 제외) & 비움영역 아님.
            auto IsRoomFloor = [&](int32 x, int32 y) -> bool
                {
                    return Grid.GetTile(x, y) == ESpartaArcadeTileType::Empty
                        && CellRoomAt(x, y) > 0
                        && !ClearCells.Contains(Grid.IndexOf(x, y));
                };

            // 구역(블록)별 스타일: 0 빈 곳 / 1 규칙 격자 / 2 어질러짐.
            TMap<int32, int32> StyleCache;
            auto StyleFor = [&](int32 bx, int32 by) -> int32
                {
                    const int32 Key = by * 100000 + bx;
                    if (int32* Found = StyleCache.Find(Key)) return *Found;
                    const float r = Rng.FRand();
                    const int32 s = (r < P.EmptyStyleWeight) ? 0
                        : (r < P.EmptyStyleWeight + P.RegularStyleWeight) ? 1 : 2;
                    StyleCache.Add(Key, s);
                    return s;
                };

            // 패스 1: 움직이지 않는 장애물(기둥 = FixedWall)
            TSet<int32> Pillars;
            for (int32 y = 0; y < H; ++y)
                for (int32 x = 0; x < W; ++x)
                {
                    if (!IsRoomFloor(x, y)) continue;
                    const int32 s = StyleFor(x / B, y / B);
                    bool bPlace = false;
                    if (s == 1) bPlace = (x % 2 == 0 && y % 2 == 0);    // 격자
                    else if (s == 2) bPlace = (Rng.FRand() < P.MessyPillarChance); // 드문드문
                    if (bPlace)
                    {
                        Grid.SetTile(x, y, ESpartaArcadeTileType::FixedWall);
                        Pillars.Add(Grid.IndexOf(x, y));
                    }
                }

            // 패스 2: 박스(DestructibleBox) — 기둥 인접이면 더 자주
            auto NearPillar = [&](int32 x, int32 y) -> bool
                {
                    const int32 NB[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
                    for (int32 d = 0; d < 4; ++d)
                    {
                        const int32 nx = x + NB[d][0], ny = y + NB[d][1];
                        if (Grid.IsInside(nx, ny) && Pillars.Contains(Grid.IndexOf(nx, ny))) return true;
                    }
                    return false;
                };
            for (int32 y = 0; y < H; ++y)
                for (int32 x = 0; x < W; ++x)
                {
                    if (!IsRoomFloor(x, y)) continue;   // 기둥 칸은 이미 FixedWall → 제외
                    const int32 s = StyleFor(x / B, y / B);
                    const float p = (s == 0) ? P.EmptyBoxChance
                        : (NearPillar(x, y) ? P.BoxDensity : P.BoxDensity * 0.45f);
                    if (Rng.FRand() < p)
                        Grid.SetTile(x, y, ESpartaArcadeTileType::DestructibleBox);
                }

            // 중앙 방(CellRoom==0): 매 판 랜덤으로 4가지 기하 패턴 중 하나.
            int32 MinX = W, MaxX = -1, MinY = H, MaxY = -1;
            for (int32 y = 0; y < H; ++y)
                for (int32 x = 0; x < W; ++x)
                    if (CellRoomAt(x, y) == 0 && Grid.GetTile(x, y) == ESpartaArcadeTileType::Empty)
                    {
                        MinX = FMath::Min(MinX, x); MaxX = FMath::Max(MaxX, x);
                        MinY = FMath::Min(MinY, y); MaxY = FMath::Max(MaxY, y);
                    }
            if (MaxX >= 0)
            {
                const int32 cw = MaxX - MinX + 1, ch = MaxY - MinY + 1;
                const int32 Pat = Rng.RandRange(0, 3);
                for (int32 y = MinY; y <= MaxY; ++y)
                    for (int32 x = MinX; x <= MaxX; ++x)
                    {
                        if (CellRoomAt(x, y) != 0 || Grid.GetTile(x, y) != ESpartaArcadeTileType::Empty) continue;
                        if (ClearCells.Contains(Grid.IndexOf(x, y))) continue;
                        const int32 t = CenterPatternTile(Pat, x - MinX, y - MinY, cw, ch);
                        if (t == 1) Grid.SetTile(x, y, ESpartaArcadeTileType::FixedWall);
                        else if (t == 2) Grid.SetTile(x, y, ESpartaArcadeTileType::DestructibleBox);
                    }
            }

            SealIsolated();
        }

        // 가장 큰 통과영역(Empty/Box) 밖의 고립 칸을 FixedWall로 메워 가짜 단절 제거.
        void SealIsolated()
        {
            const int32 W = P.Width, H = P.Height, N = W * H;
            auto Passable = [&](int32 x, int32 y)
                {
                    const ESpartaArcadeTileType T = Grid.GetTile(x, y);
                    return T == ESpartaArcadeTileType::Empty || T == ESpartaArcadeTileType::DestructibleBox;
                };
            TArray<int32> Comp; Comp.Init(-1, N);
            TArray<int32> CompSize;
            const int32 NB[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
            int32 NumComp = 0;
            for (int32 sy = 0; sy < H; ++sy)
                for (int32 sx = 0; sx < W; ++sx)
                {
                    if (!Passable(sx, sy)) continue;
                    const int32 SIdx = Grid.IndexOf(sx, sy);
                    if (Comp[SIdx] >= 0) continue;
                    const int32 Id = NumComp++;
                    int32 Size = 0;
                    TArray<int32> Stack; Stack.Push(SIdx); Comp[SIdx] = Id;
                    while (Stack.Num() > 0)
                    {
                        const int32 Ci = Stack.Pop(); ++Size;
                        const int32 cx = Ci % W, cy = Ci / W;
                        for (int32 d = 0; d < 4; ++d)
                        {
                            const int32 nx = cx + NB[d][0], ny = cy + NB[d][1];
                            if (!Grid.IsInside(nx, ny) || !Passable(nx, ny)) continue;
                            const int32 Ni = Grid.IndexOf(nx, ny);
                            if (Comp[Ni] >= 0) continue;
                            Comp[Ni] = Id; Stack.Push(Ni);
                        }
                    }
                    CompSize.Add(Size);
                }
            if (NumComp <= 1) return;
            int32 Best = 0;
            for (int32 i = 1; i < NumComp; ++i) if (CompSize[i] > CompSize[Best]) Best = i;
            for (int32 i = 0; i < N; ++i)
                if (Comp[i] >= 0 && Comp[i] != Best)
                    Grid.SetTile(i % W, i / W, ESpartaArcadeTileType::FixedWall);
        }

        // 4사분면 강제: 각 모서리에 가장 가까운 방 칸을 스폰으로, 주변(같은 방) 사각 반경을 비움.
        void PlaceSpawns(int32 SafeRadius)
        {
            const int32 W = P.Width, H = P.Height;
            const FIntPoint Corners[4] = { FIntPoint(0,0), FIntPoint(W - 1,0), FIntPoint(0,H - 1), FIntPoint(W - 1,H - 1) };
            const int32 HX = W / 2, HY = H / 2;
            Spawns.Reset();
            for (int32 q = 0; q < 4; ++q)
            {
                const FIntPoint& Cn = Corners[q];
                int32 BestX = -1, BestY = -1, BestD = MAX_int32;
                for (int32 y = 0; y < H; ++y)
                    for (int32 x = 0; x < W; ++x)
                    {
                        if (CellRoomAt(x, y) < 0) continue;
                        const bool bInQ =
                            (q == 0 && x < HX && y < HY) || (q == 1 && x >= HX && y < HY) ||
                            (q == 2 && x < HX && y >= HY) || (q == 3 && x >= HX && y >= HY);
                        if (!bInQ) continue;
                        const int32 d = FMath::Abs(x - Cn.X) + FMath::Abs(y - Cn.Y);
                        if (d < BestD) { BestD = d; BestX = x; BestY = y; }
                    }
                if (BestX >= 0) Spawns.Add(FIntPoint(BestX, BestY));
            }
            // 안전구역: 스폰 중심 사각 반경(같은 방)만 기둥/박스 → 바닥
            for (const FIntPoint& Sp : Spawns)
            {
                const int32 Rid = CellRoomAt(Sp.X, Sp.Y);
                for (int32 y = FMath::Max(0, Sp.Y - SafeRadius); y <= FMath::Min(H - 1, Sp.Y + SafeRadius); ++y)
                    for (int32 x = FMath::Max(0, Sp.X - SafeRadius); x <= FMath::Min(W - 1, Sp.X + SafeRadius); ++x)
                    {
                        // 스폰 지점 주변 3x3 영역(반경 1)은 방 ID가 다르거나 Void여도 무조건 비우고, 그 외 안전 반경은 동일 방일 때만 비움
                        const bool bIn3x3 = FMath::Max(FMath::Abs(x - Sp.X), FMath::Abs(y - Sp.Y)) <= 1;
                        if (!bIn3x3 && CellRoomAt(x, y) != Rid) continue;

                        const ESpartaArcadeTileType T = Grid.GetTile(x, y);
                        if (T == ESpartaArcadeTileType::FixedWall)
                        {
                            // 고정벽은 3x3 영역이더라도 비우지 않고 유지, 오직 스폰 포인트 1칸(x == Sp.X && y == Sp.Y)만 비움
                            if (x == Sp.X && y == Sp.Y)
                            {
                                Grid.SetTile(x, y, ESpartaArcadeTileType::Empty);
                            }
                        }
                        else if (T == ESpartaArcadeTileType::DestructibleBox || T == ESpartaArcadeTileType::Void)
                        {
                            // Void 타일 또한 3x3 영역이더라도 비우지 않고 유지, 오직 스폰 포인트 1칸(x == Sp.X && y == Sp.Y)만 비움
                            // 박스(DestructibleBox)는 기존대로 3x3 안전구역 내에서도 통로 확보를 위해 빈 공간으로 변환
                            if (T == ESpartaArcadeTileType::DestructibleBox || (x == Sp.X && y == Sp.Y))
                            {
                                Grid.SetTile(x, y, ESpartaArcadeTileType::Empty);
                                if (bIn3x3 && CellRoomAt(x, y) < 0)
                                {
                                    CellRoom[Grid.IndexOf(x, y)] = Rid;
                                }
                            }
                        }
                    }
                Grid.SetTile(Sp.X, Sp.Y, ESpartaArcadeTileType::Empty);
            }
        }

        void PlaceVariants(float Coverage, bool bInCenter, int32 SafeRadius)
        {
            if (Coverage <= 0.f) return;
            const int32 W = P.Width, H = P.Height;

            // 스폰 안전구역 보호(그 위엔 변형 타일 안 깜)
            TSet<int32> Protect;
            for (const FIntPoint& Sp : Spawns)
                for (int32 y = FMath::Max(0, Sp.Y - SafeRadius); y <= FMath::Min(H - 1, Sp.Y + SafeRadius); ++y)
                    for (int32 x = FMath::Max(0, Sp.X - SafeRadius); x <= FMath::Min(W - 1, Sp.X + SafeRadius); ++x)
                        if (FMath::Max(FMath::Abs(x - Sp.X), FMath::Abs(y - Sp.Y)) <= SafeRadius)
                            Protect.Add(Grid.IndexOf(x, y));

            // 대상 = 빈 바닥(기둥·박스·벽·문 제외) & (토글 시)중앙 제외 & 안전구역 제외
            auto Eligible = [&](int32 x, int32 y) -> bool
                {
                    if (Grid.GetTile(x, y) != ESpartaArcadeTileType::Empty) return false;
                    const int32 Rid = CellRoomAt(x, y);
                    if (Rid < 0) return false;                  // 문 통로
                    if (!bInCenter && Rid == 0) return false;   // 중앙 아레나(토글)
                    if (Protect.Contains(Grid.IndexOf(x, y))) return false;
                    return true;
                };

            // 고정벽(FixedWall) 또는 맵 외부(Void)와 인접한지 검사하는 헬퍼 람다
            auto HasWallNeighbor = [&](int32 x, int32 y) -> bool
            {
                static const int32 DX[] = { 1, -1, 0, 0 };
                static const int32 DY[] = { 0, 0, 1, -1 };
                for (int32 d = 0; d < 4; ++d)
                {
                    int32 nx = x + DX[d];
                    int32 ny = y + DY[d];
                    if (Grid.IsInside(nx, ny))
                    {
                        ESpartaArcadeTileType T = Grid.GetTile(nx, ny);
                        if (T == ESpartaArcadeTileType::FixedWall || T == ESpartaArcadeTileType::Void)
                        {
                            return true;
                        }
                    }
                    else
                    {
                        return true; // 맵 밖도 경계벽 취급
                    }
                }
                return false;
            };

            int32 Total = 0;
            for (int32 y = 0; y < H; ++y)
                for (int32 x = 0; x < W; ++x)
                    if (Eligible(x, y)) ++Total;
            const int32 Target = FMath::RoundToInt(Total * Coverage);
            if (Target <= 0) return;

            const ESpartaArcadeTileType Types[3] = {
                ESpartaArcadeTileType::Ice, ESpartaArcadeTileType::MudWater, ESpartaArcadeTileType::Bush };

            auto FindSeed = [&](int32& OX, int32& OY) -> bool
                {
                    for (int32 i = 0; i < 40; ++i)
                    {
                        const int32 SX = Rng.RandRange(0, W - 1), SY = Rng.RandRange(0, H - 1);
                        if (Eligible(SX, SY)) { OX = SX; OY = SY; return true; }
                    }
                    return false;
                };

            const int32 NB[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
            TArray<FIntPoint> Frontier;
            int32 Placed = 0, Guard = 0;
            while (Placed < Target && Guard < 6000)
            {
                ++Guard;
                int32 SX, SY;
                if (!FindSeed(SX, SY)) break;

                // Seed 단계에서 Bush가 벽 옆에 배치되려 하면 Ice 또는 Mud로 선회
                ESpartaArcadeTileType VT = Types[Rng.RandRange(0, 2)];
                if (VT == ESpartaArcadeTileType::Bush && HasWallNeighbor(SX, SY))
                {
                    VT = (Rng.RandRange(0, 1) == 0) ? ESpartaArcadeTileType::Ice : ESpartaArcadeTileType::MudWater;
                }

                const int32 Size = Rng.RandRange(4, 11);
                Grid.SetTile(SX, SY, VT); ++Placed;
                Frontier.Reset(); Frontier.Add(FIntPoint(SX, SY));
                int32 Count = 1;
                while (Frontier.Num() > 0 && Count < Size)
                {
                    const int32 Pick = Rng.RandRange(0, Frontier.Num() - 1);
                    const FIntPoint C = Frontier[Pick];
                    Frontier.RemoveAtSwap(Pick);
                    for (int32 d = 0; d < 4; ++d)
                    {
                        const int32 nx = C.X + NB[d][0], ny = C.Y + NB[d][1];
                        if (Grid.IsInside(nx, ny) && Eligible(nx, ny))
                        {
                            // 부시(Bush) 확장 시 고정벽/경계와 닿아 겹치는 문제가 없도록 해당 방향 확장을 건너뜀
                            if (VT == ESpartaArcadeTileType::Bush && HasWallNeighbor(nx, ny))
                            {
                                continue;
                            }
                            Grid.SetTile(nx, ny, VT); Frontier.Add(FIntPoint(nx, ny));
                            ++Count; ++Placed;
                            if (Count >= Size) break;
                        }
                    }
                }
            }
        }

        // 이동 장애물 스폰 위치 추출(그리드 조회용). 중앙·문·스폰 안전구역 제외, 열린 바닥에서 N개 랜덤.
        void PickObstacleSpawns(int32 Count, int32 SafeRadius)
        {
            ObstacleSpawns.Reset();
            if (Count <= 0) return;
            const int32 W = P.Width, H = P.Height;

            TSet<int32> Protect;
            for (const FIntPoint& Sp : Spawns)
                for (int32 y = FMath::Max(0, Sp.Y - SafeRadius); y <= FMath::Min(H - 1, Sp.Y + SafeRadius); ++y)
                    for (int32 x = FMath::Max(0, Sp.X - SafeRadius); x <= FMath::Min(W - 1, Sp.X + SafeRadius); ++x)
                        Protect.Add(Grid.IndexOf(x, y));

            auto Walkable = [](ESpartaArcadeTileType T)
                {
                    return T == ESpartaArcadeTileType::Empty || T == ESpartaArcadeTileType::Ice
                        || T == ESpartaArcadeTileType::MudWater || T == ESpartaArcadeTileType::Bush;
                };

            TArray<FIntPoint> Cand;
            for (int32 y = 0; y < H; ++y)
                for (int32 x = 0; x < W; ++x)
                {
                    const int32 Rid = CellRoomAt(x, y);
                    if (Rid <= 0) continue;                        // 문(-1)·중앙(0) 제외
                    if (!Walkable(Grid.GetTile(x, y))) continue;   // 벽·박스·기둥 제외
                    if (Protect.Contains(Grid.IndexOf(x, y))) continue;
                    Cand.Add(FIntPoint(x, y));
                }

            // 셔플 후 앞에서 N개
            for (int32 i = Cand.Num() - 1; i > 0; --i)
            {
                const int32 j = Rng.RandRange(0, i);
                Cand.Swap(i, j);
            }
            const int32 Take = FMath::Min(Count, Cand.Num());
            for (int32 i = 0; i < Take; ++i) ObstacleSpawns.Add(Cand[i]);
        }

        // 중앙 아레나(CellRoom==0) 셀의 bbox 계산 → 자기장 최종 지대로 사용.
        void ComputeCenterBounds()
        {
            const int32 W = P.Width, H = P.Height;
            CenterMin = FIntPoint(W, H);
            CenterMax = FIntPoint(-1, -1);
            for (int32 y = 0; y < H; ++y)
                for (int32 x = 0; x < W; ++x)
                    if (CellRoomAt(x, y) == 0)
                    {
                        CenterMin.X = FMath::Min(CenterMin.X, x);
                        CenterMin.Y = FMath::Min(CenterMin.Y, y);
                        CenterMax.X = FMath::Max(CenterMax.X, x);
                        CenterMax.Y = FMath::Max(CenterMax.Y, y);
                    }
            if (CenterMax.X < 0) // 중앙이 없으면 맵 정중앙 폴백
            {
                CenterMin = CenterMax = FIntPoint(W / 2, H / 2);
            }
        }
    };
}

void FSpartaArcadeRoomGenerator::Generate(FSpartaArcadeMapGrid& OutGrid, int32 Seed,
    const FSpartaArcadeRoomGenParams& Params,
    TArray<FIntPoint>* OutSpawns,
    TArray<FIntPoint>* OutObstacleSpawns,
    FIntPoint* OutCenterMin, FIntPoint* OutCenterMax)
{
    FRoomGenImpl Impl(OutGrid, Seed, Params);
    Impl.Run();
    if (OutSpawns) *OutSpawns = Impl.Spawns;
    if (OutObstacleSpawns) *OutObstacleSpawns = Impl.ObstacleSpawns;
    if (OutCenterMin) *OutCenterMin = Impl.CenterMin;
    if (OutCenterMax) *OutCenterMax = Impl.CenterMax;
}

int32 FSpartaArcadeRoomGenerator::CountReachableNonWall(const FSpartaArcadeMapGrid& Grid,
    int32 StartX, int32 StartY)
{
    auto Blocked = [](ESpartaArcadeTileType T)
        {
            return T == ESpartaArcadeTileType::FixedWall || T == ESpartaArcadeTileType::Void;
        };
    if (Blocked(Grid.GetTile(StartX, StartY))) return 0;

    TArray<bool> Seen;
    Seen.Init(false, Grid.Width * Grid.Height);
    TArray<FIntPoint> Stack;
    Stack.Push(FIntPoint(StartX, StartY));
    Seen[Grid.IndexOf(StartX, StartY)] = true;

    const int32 DX[4] = { 1, -1, 0, 0 };
    const int32 DY[4] = { 0, 0, 1, -1 };

    int32 Count = 0;
    while (Stack.Num() > 0)
    {
        const FIntPoint C = Stack.Pop();
        ++Count;
        for (int32 d = 0; d < 4; ++d)
        {
            const int32 nx = C.X + DX[d], ny = C.Y + DY[d];
            if (!Grid.IsInside(nx, ny)) continue;
            const int32 Idx = Grid.IndexOf(nx, ny);
            if (Seen[Idx]) continue;
            if (Blocked(Grid.GetTile(nx, ny))) continue;
            Seen[Idx] = true;
            Stack.Push(FIntPoint(nx, ny));
        }
    }
    return Count;
}