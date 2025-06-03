以下は 2025年6月時点で **公開状態** にあり，学生フォーミュラ／大学EVプロジェクトのパワートレイン系ソフトウェアが閲覧できる GitHub リポジトリの代表例です。いずれも教育・研究目的で ASIL 認証は前提にしていませんが，FreeRTOS／ChibiOS／Simulink 自動生成コードなど実運用レベルの実装が含まれています。

| # | チーム / 大学                                        | 主要リポジトリ                                     | ハード構成・OS/RTOSの特徴                                   | 概要                                                                     |
| - | ----------------------------------------------- | ------------------------------------------- | -------------------------------------------------- | ---------------------------------------------------------------------- |
| 1 | **KTH Formula Student (スウェーデン)**                | `kthfspe/bldc`<br>（VESC fork）               | STM32F4 + **ChibiOS**、CAN経由で自車データを拡張               | FOC/電流制御に独自 CAN ID を追加し，VESC Tool では扱えない車両固有信号を統合 ([GitHub][1])        |
| 2 | **Northeastern Electric Racing (米国)**           | Org 配下に `VCU-Firmware` / `BMS-Firmware` など  | STM32F407 + **FreeRTOS**、CANopen スタック              | ドライバ入力→トルク要求・BMS 監視を 1 kHz で実装。PlatformIO ＋ CI ワークフローを公開 ([GitHub][2]) |
| 3 | **Black Forest Formula Team (独・オッフェンブルク応用科学大)** | `Black-Forest-Formula-Team/VCU-STM32F767ZI` | STM32F767 + HAL（素の RT 割込み）、PlatformIO              | 600 V エミュレータを想定したデュアル CAN 経路とプリチャージ制御ロジックをソース付きで公開 ([GitHub][3])       |
| 4 | **SUFST (南ア・Stellenbosch Uni)**                 | `sufst/vcu`                                 | STM32F746 + **FreeRTOS**、CAN FD                    | ドライバモード切替・トルクベクタリングをタスク分割し，GoogleTest でユニットテストも併設 ([GitHub][4])        |
| 5 | **Aristurtle (ギリシャ・Aristotle Uni)**             | `vamoirid/ECU-Racing-2019-dSPACE-R2017a`    | **dSPACE MicroAutoBox II**（独自 RTOS）+ Simulink 自動生成 | 400 Hz で車両状態推定，dSPACE ControlDesk 用 A2L ファイルも同梱 ([GitHub][5])          |
| 6 | **NU Formula Racing (タイ・Chulalongkorn Uni)**    | `NU-Formula-Racing/VCU-2023-24`             | STM32G4 + **Zephyr OS** 移植を試行中                     | PlatformIO ベース。アクセルマップと故障診断を Zephyr のスレッドに整理 ([GitHub][6])             |
| 7 | **UMSAE Formula Electric (カナダ・マニトバ大)**          | `UMSAE-Formula-Electric/VCU2.0`             | TI F28379D + **TI-RTOS (SYS/BIOS)**                | Dual-Core C2000 の片方で 20 kHz FOC，もう一方で車両統合 CAN を処理 ([GitHub][7])        |

### 使い方のヒント

* **実装の雰囲気をつかむ**：FreeRTOS や HAL レイヤの使い方，ドライバ入力フィルタリング，プリチャージ手順など，量産 ECU と同様の構造を学生向けに簡素化した例が多い。
* **安全対策の参考**：フォールトステート遷移図や CAN DBC ファイルが付属しているケースがあり，競技レギュレーションへの適合方法を学べる。
* **自プロジェクトへの流用**：プルリク歓迎としているチームも多いが，学術ライセンス（GPL-3.0 や BSD-3）かどうかを必ず確認し，著作権表示を保持すること。

これらのリポジトリをクローンして動かすだけでも，学生 EV のパワートレイン制御設計の全体像を体験できます。

[1]: https://github.com/orgs/kthfspe/repositories?utm_source=chatgpt.com "kthfspe repositories · GitHub"
[2]: https://github.com/Northeastern-Electric-Racing?utm_source=chatgpt.com "Northeastern Electric Racing - GitHub"
[3]: https://github.com/Black-Forest-Formula-Team/VCU-STM32F767ZI?utm_source=chatgpt.com "Black-Forest-Formula-Team/VCU-STM32F767ZI: For the ... - GitHub"
[4]: https://github.com/sufst/vcu?utm_source=chatgpt.com "sufst/vcu: Embedded software for the VCU - GitHub"
[5]: https://github.com/vamoirid/ECU-Racing-2019-dSPACE-R2017a?utm_source=chatgpt.com "vamoirid/ECU-Racing-2019-dSPACE-R2017a - GitHub"
[6]: https://github.com/NU-Formula-Racing/VCU-2023-24?utm_source=chatgpt.com "NU-Formula-Racing/VCU-2023-24 - GitHub"
[7]: https://github.com/UMSAE-Formula-Electric/VCU2.0?utm_source=chatgpt.com "Vehicle Control Unit (VCU) for the UMSAE formula electric car - GitHub"


# CAN DBC ファイルとは？
### 概要

**CAN DBC ファイル**は、CAN バス上を流れるメッセージ（フレーム）とその中のシグナル（ビット列）を**人間が読めるテキスト形式**で記述した “辞書” です。
1990 年代に Vector Informatik が CANdb++ 用に定義したフォーマットが事実上の業界標準となり、サプライヤ間・ツール間で ECU の CAN 仕様を受け渡す際に広く使われています。

---

### 役割

| 機能          | 具体的内容                                                                  |
| ----------- | ---------------------------------------------------------------------- |
| **メッセージ定義** | ID（11 bit/29 bit）、送信周期、データ長 DLC、送信 ECU 名など                             |
| **シグナル定義**  | 開始ビット位置、ビット長、符号付き・符号なし、エンディアン、係数（スケール）とオフセット、物理単位、最小/最大値、送信 ECU、受信 ECU |
| **属性・コメント** | デフォルト値、故障値、診断フラグ、トリガ条件、自由記述のコメント                                       |
| **値テーブル**   | 例：0=OFF, 1=ON のような列挙型ラベル                                               |

これらを 1 つの DBC にまとめることで、

* **解析ツールが自動でビット列→物理値へ変換**
* **実装側（ECU ファームウェア、PC 解析ソフト、HIL テストベンチ）が同じ定義を共有**
* **バージョン管理（Git など）で変更点を差分追跡**

できるようになります。

---

### 最低限知っておくべき構文

```dbc
VERSION "1.0"

BO_ 1234 MotorStatus: 8 Vector_ECU
 SG_ RPM            :  0|16@1+ (0.125, 0) [0|8000]  "rpm"  Vector_ECU
 SG_ Temperature    : 16|8 @1+ (1,     -40) [-40|215] "°C"  Vector_ECU
 SG_ ErrorFlag      : 24|1 @1+ (1,      0) [0|1]     ""    Vector_ECU
```

*`BO_` 行がメッセージ、`SG_` 行がシグナル*

* `0|16@1+` → 開始ビット 0、長さ 16 bit、Intel リトルエンディアン、符号なし
* `(0.125,0)` → 物理値 = raw × 0.125 + 0

---

### 主な編集・利用ツール（代表例）

| 種別  | 代表的ツール                                                         | 備考                                    |
| --- | -------------------------------------------------------------- | ------------------------------------- |
| 商用  | Vector CANdb++, CANape, ETAS INCA, PEAK PCAN-Explorer          | GUI 編集・計測・フラッシュ書き込みまでワンストップ           |
| OSS | **cantools** (Python), **Busmaster**, **Kayak**, **canmatrix** | CI で DBC → C/C++/Rust コード生成や自動テストに活用可 |

---

### 他フォーマットとの関係

| フォーマット          | 立ち位置                                 |
| --------------- | ------------------------------------ |
| ARXML (AUTOSAR) | XML でより詳細に定義。量産 ECU 間の主流だが人が直接読むには冗長 |
| KCD (Kvaser)    | XML 版 DBC。オープンだが採用は限定的               |
| SYM (Peak)      | シンプルで学習用途向け。ただし多バイト信号には不向き           |

---

### まとめ

* **DBC = CAN メッセージの “仕様書かつ機械可読データベース”**
* 学生チームでも **cantools** などで簡単に扱え、ファームウェアと解析 GUI を同じ DBC から自動生成することで **仕様ズレを防止** できる。
* 研究・実験段階でも早めに DBC を管理リポジトリに含めておくと、後から ECU や解析ソフトを増やす際に大幅な手戻りを防げます。
