> Historical audit / 历史核对记录：部分按键与校准状态已变更。当前配置以 `keymap/gameforpeace-k80-f8.json` 和 `AI_CONTRIBUTION.md` 为准；此文不代表最终真机通过。

# K80 mapping audit 2026-10-22

Coordinates refer to the supplied HUD screenshot, not verified current gameplay.

| Action | Key | Position | Type | Cursor mode |
|---|---|---|---|---|
| 方向盘 | W/A/S/D | {'x': 0.2, 'y': 0.695} | KMT_STEER_WHEEL | False |
| 左探头 | Key_Q | {'x': 0.154, 'y': 0.358} | KMT_CLICK | False |
| 右探头 | Key_E | {'x': 0.222, 'y': 0.362} | KMT_CLICK | False |
| 自动跑 | Key_Equal | {'x': 0.2, 'y': 0.215} | KMT_CLICK | False |
| 跳 | Key_Space | {'x': 0.92, 'y': 0.694} | KMT_CLICK | False |
| 地图 | Key_M | {'x': 0.899, 'y': 0.122} | KMT_CLICK | True |
| 背包 | Key_Tab | {'x': 0.103, 'y': 0.909} | KMT_CLICK | True |
| 视角 | Key_V | {'x': 0.229, 'y': 0.946} | KMT_CLICK | False |
| 趴 | Key_Z | {'x': 0.908, 'y': 0.899} | KMT_CLICK | False |
| 蹲 | Key_C | {'x': 0.83, 'y': 0.93} | KMT_CLICK | False |
| 换弹 | Key_R | {'x': 0.763, 'y': 0.934} | KMT_CLICK | False |
| 换枪1 | Key_1 | {'x': 0.445, 'y': 0.883} | KMT_CLICK | False |
| 换枪2 | Key_2 | {'x': 0.552, 'y': 0.883} | KMT_CLICK | False |
| 手雷 | Key_3 | {'x': 0.65, 'y': 0.932} | KMT_CLICK | False |
| 快速打药 | Key_4 | {'x': 0.347, 'y': 0.934} | KMT_CLICK | False |
| 下车 | Key_5 | {'x': 0.885, 'y': 0.382} | KMT_CLICK | False |
| 救人 | Key_6 | {'x': 0.5, 'y': 0.611} | KMT_CLICK | False |
| 手枪 | Key_7 | {'x': 0.59, 'y': 0.823} | KMT_CLICK | False |
| 投掷物菜单 | Key_F1 | {'x': 0.65, 'y': 0.868} | KMT_CLICK | True |
| 药物菜单 | Key_F2 | {'x': 0.348, 'y': 0.868} | KMT_CLICK | True |
| 消息菜单 | Key_F3 | {'x': 0.89, 'y': 0.225} | KMT_CLICK | True |
| 表情菜单 | Key_F4 | {'x': 0.795, 'y': 0.033} | KMT_CLICK | True |
| 开关门 | Key_X | {'x': 0.617, 'y': 0.543} | KMT_CLICK | False |
| 开枪 | LeftButton | {'x': 0.111, 'y': 0.54} | KMT_CLICK | False |
| 开镜 | RightButton | {'x': 0.928, 'y': 0.498} | KMT_CLICK | False |
| 开关门（F，X保留） | Key_F | {'x': 0.617, 'y': 0.543} | KMT_CLICK | False |
| 准镜选择入口（选项待校准） | Key_Y | {'x': 0.955, 'y': 0.299} | KMT_CLICK | True |
| 驾驶 | Key_F5 | {'x': 0.617, 'y': 0.343} | KMT_CLICK | False |
| 乘坐 | Key_F6 | {'x': 0.617, 'y': 0.46} | KMT_CLICK | False |

Confirmed defects: Q/E silently rejected due to missing switchMap; Tab focus traversal; repeated 反引号（` / ～）; same-coordinate panel close.

Not calibrated: pickup list/toggle/items, individual medicine selection, scope choices and adjustable scope slider. Need current expanded screenshots.

Q/E hold vs toggle depends on in-game settings. Tab/M close uses Android Back; its game behavior needs acceptance. Manual menu dismissal does not synchronize the local state. 反引号（` / ～） off/on resets it.

F = door (X retained), F5 = drive, F6 = passenger, Y = scope-menu entry; new coordinates inferred from old HUD. F is not context-aware pickup/drive/rescue.

All existing controls are included above. No macro, auto-fire or automated pickup added.
