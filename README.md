# SpartaArcade (UE5 Multiplayer Bomber Game)

![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.4+-0E1128?logo=unrealengine&logoColor=white)
![C++](https://img.shields.io/badge/C++-17%20/%2020-00599C?logo=c%2B%2B&logoColor=white)
![Epic Online Services](https://img.shields.io/badge/Epic%20Online%20Services-EOS-blue)

**SpartaArcade**는 Unreal Engine 5와 C++를 기반으로 개발된 **멀티플레이어 아케이드 봄버(Bomber) 게임**입니다. 클래식 봄버맨 스타일의 규칙에 현대적인 **GAS(Gameplay Ability System)** 아키텍처와 **EOS(Epic Online Services)** 멀티플레이어 매칭 환경을 결합하였습니다.

---

## 🎮 Key Features (핵심 기능)

1. **GAS 기반 능력 및 아이템 시스템**
   * 폭탄 설치, 구급상자 자가회복, 쉴드 방어막 활성화 등 다양한 액션을 Gameplay Ability System으로 유연하고 강력하게 처리합니다.
   * 복제(Replication)를 지원하는 `AttributeSet`을 통해 속도, 폭탄 범위, 보유 한도 등의 실시간 스탯 변화가 지연 없이 모든 클라이언트에 동기화됩니다.

2. **EOS(Epic Online Services) 멀티플레이 세션**
   * Dedicated Server/Listen Server 환경에서 세션 생성, 검색 및 가입을 원활히 지원합니다.
   * 심리스 트래블(Seamless Travel)을 적용하여 로비에서 인게임 맵으로 로딩 끊김 없이 전원이 동시에 안전하게 이동합니다.

3. **실시간 팀/솔로 랭킹 산정 및 매치 결과 시스템**
   * 서바이벌 방식의 탈락 연산을 실시간으로 처리합니다.
   * 플레이어가 죽을 때마다 실시간으로 살아있는 플레이어 수와 팀 수를 계산하여 최종 스코어 및 랭킹 결과를 리더보드에 업데이트합니다.

4. **실시간 관전(Spectate) 기능**
   * 게임에서 조기 탈락하더라도 다른 플레이어들을 관전할 수 있는 모드를 제공합니다.
   * 뷰타겟 전환 및 대상 플레이어의 체력/실드/스탯 정보를 내 HUD 화면에 실시간으로 바인딩하여 밀착도 높은 관전 경험을 제공합니다.

---

## 🛠️ Tech Stack (사용 기술 및 환경)

* **Game Engine**: Unreal Engine 5 (v5.4+)
* **Programming**: C++ 17/20, Blueprints
* **Framework**: Gameplay Ability System (GAS), Enhanced Input System
* **Network & Matchmaking**: Epic Online Services (EOS) SDK
* **UI**: UMG (Unreal Motion Graphics) C++ bindings

---

## 📁 Project Structure (프로젝트 주요 구조)

```text
Source/SpartaArcade/
├── Characters/         # 플레이어 캐릭터 및 입력/조작 처리를 위한 C++ 컨트롤러 클래스
│   ├── Public/Private/SpartaArcadeCharacter
│   └── Public/Private/SpartaArcadePlayerController
├── Framework/          # 게임 모드, 상태 관리 및 세션 전환 핵심 프레임워크
│   ├── InGame/         # 인게임 규칙 (SpartaGameMode, SpartaGameState, SpartaPlayerState)
│   └── Lobby/          # 로비 규칙 (LobbyGameMode, LobbyPlayerController)
├── Systems/            # GAS 어빌리티(폭탄 설치 등) 및 어트리뷰트 셋 구현
│   └── Public/Private/GA_PlaceBomb, BomberAttributeSet
└── UI/                 # HUD 메인 게이지 및 타이틀/로비/일시정지/결과창 UI 컨트롤러
    └── Public/Private/SpartaHUDWidget, SpartaMenuFlowWidget
```

---
