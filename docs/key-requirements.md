# 按键需求表

> 填写说明：每个页面按相同矩阵列出所有业务按键。`-` 表示该页面不处理该按键事件；如页面只处理部分事件，其他事件保持 `-`。

## 按键事件模板

| 按键                 | Click         | Press | Release | Long Press | Long Press 3S | Long Press 3S Release | Long Press Repeat |
| -------------------- | ------------- | ----- | ------- | ---------- | ------------- | --------------------- | ----------------- |
| `KEY_ID_POWER`       | 全局亮屏/熄屏 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_ASSISTANT`   | 按页面定义    | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_UP`   | 按页面定义    | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_DOWN` | 按页面定义    | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_FOCUS`       | 按页面定义    | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_CAMERA`      | 按页面定义    | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MODE`        | 按页面定义    | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MENU`        | 按页面定义    | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_UP`          | 按页面定义    | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_DOWN`        | 按页面定义    | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_LEFT`        | 按页面定义    | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_RIGHT`       | 按页面定义    | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_OK`          | 按页面定义    | -     | -       | -          | -             | -                     | -                 |

## 全局按键

| 按键                 | Click     | Press | Release | Long Press | Long Press 3S | Long Press 3S Release | Long Press Repeat |
| -------------------- | --------- | ----- | ------- | ---------- | ------------- | --------------------- | ----------------- |
| `KEY_ID_POWER`       | 亮屏/熄屏 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_ASSISTANT`   | -         | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_UP`   | -         | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_DOWN` | -         | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_FOCUS`       | -         | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_CAMERA`      | -         | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MODE`        | -         | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MENU`        | -         | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_UP`          | -         | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_DOWN`        | -         | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_LEFT`        | -         | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_RIGHT`       | -         | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_OK`          | -         | -     | -       | -          | -             | -                     | -                 |

说明：
1. `ANY_KEY + ANY_EVENT` 全局刷新自动休眠计时。
2. `KEY_ID_VOLUME_UP/DOWN` 不在 `key_manager` 中注册默认音量逻辑。

## 页面按键

### 首页 (`page_home`)

| 按键                 | Click            | Press | Release | Long Press | Long Press 3S | Long Press 3S Release | Long Press Repeat |
| -------------------- | ---------------- | ----- | ------- | ---------- | ------------- | --------------------- | ----------------- |
| `KEY_ID_POWER`       | -                | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_ASSISTANT`   | -                | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_UP`   | -                | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_DOWN` | -                | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_FOCUS`       | -                | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_CAMERA`      | -                | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MODE`        | -                | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MENU`        | -                | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_UP`          | 向上选中         | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_DOWN`        | 向下选中         | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_LEFT`        | 向左选中         | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_RIGHT`       | 向右选中         | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_OK`          | 进入当前选中页面 | -     | -       | -          | -             | -                     | -                 |

### 拍照页 (`page_photo`)

| 按键                 | Click                                    | Press                | Release                  | Long Press                           | Long Press 3S              | Long Press 3S Release | Long Press Repeat |
| -------------------- | ---------------------------------------- | -------------------- | ------------------------ | ------------------------------------ | -------------------------- | --------------------- | ----------------- |
| `KEY_ID_POWER`       | -                                        | -                    | -                        | -                                    | -                          | -                     | -                 |
| `KEY_ID_ASSISTANT`   | 打开/关闭滤镜面板                        | -                    | -                        | -                                    | -                          | -                     | -                 |
| `KEY_ID_VOLUME_UP`   | 放大变焦（1→2→3→6）                      | -                    | -                        | -                                    | -                          | -                     | -                 |
| `KEY_ID_VOLUME_DOWN` | 缩小变焦（6→3→2→1）                      | -                    | -                        | -                                    | -                          | -                     | -                 |
| `KEY_ID_FOCUS`       | -                                        | 对焦一次并显示对焦框 | 重新开启 AF 并隐藏对焦框 | -                                    | 关闭 AF 并显示锁定态对焦框 | -                     | -                 |
| `KEY_ID_CAMERA`      | 拍照                                     | -                    | -                        | -                                    | -                          | -                     | -                 |
| `KEY_ID_MODE`        | 切换到录像模式                           | -                    | -                        | -                                    | -                          | -                     | -                 |
| `KEY_ID_MENU`        | 打开拍照设置页；滤镜面板打开时先关闭面板 | -                    | -                        | 返回上一页；滤镜面板打开时先关闭面板 | -                          | -                     | -                 |
| `KEY_ID_UP`          | -                                        | -                    | -                        | -                                    | -                          | -                     | -                 |
| `KEY_ID_DOWN`        | -                                        | -                    | -                        | -                                    | -                          | -                     | -                 |
| `KEY_ID_LEFT`        | 滤镜面板打开时选择上一个滤镜并应用       | -                    | -                        | -                                    | -                          | -                     | -                 |
| `KEY_ID_RIGHT`       | 滤镜面板打开时选择下一个滤镜并应用       | -                    | -                        | -                                    | -                          | -                     | -                 |
| `KEY_ID_OK`          | -                                        | -                    | -                        | -                                    | -                          | -                     | -                 |

补充：
1. 滤镜面板打开后，除 `LEFT/RIGHT Click` 外的其他物理键事件优先关闭面板，不继续执行页面动作。

### 拍照设置 (`page_photo_settings`)

| 按键                 | Click                                           | Press | Release | Long Press | Long Press 3S | Long Press 3S Release | Long Press Repeat |
| -------------------- | ----------------------------------------------- | ----- | ------- | ---------- | ------------- | --------------------- | ----------------- |
| `KEY_ID_POWER`       | -                                               | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_ASSISTANT`   | -                                               | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_UP`   | -                                               | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_DOWN` | -                                               | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_FOCUS`       | -                                               | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_CAMERA`      | -                                               | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MODE`        | -                                               | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MENU`        | 返回上一页；滚轮弹窗打开时关闭弹窗              | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_UP`          | 上移选中；首尾循环；滚轮弹窗打开时选择上一项    | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_DOWN`        | 下移选中；首尾循环；滚轮弹窗打开时选择下一项    | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_LEFT`        | -                                               | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_RIGHT`       | -                                               | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_OK`          | 打开当前项滚轮/切换开关；滚轮弹窗打开时应用更改 | -     | -       | -          | -             | -                     | -                 |

补充：
1. 当前选中项变化时必须自动滚动到可见范围。
2. `VOLUME_UP/VOLUME_DOWN` 默认不处理，不能触发变焦或系统音量。
3. 如需长按连发滚动，应由页面显式注册 `Long Press Repeat`。

### 录像页 (`page_video`)

| 按键                 | Click                                    | Press | Release | Long Press                           | Long Press 3S | Long Press 3S Release | Long Press Repeat |
| -------------------- | ---------------------------------------- | ----- | ------- | ------------------------------------ | ------------- | --------------------- | ----------------- |
| `KEY_ID_POWER`       | -                                        | -     | -       | -                                    | -             | -                     | -                 |
| `KEY_ID_ASSISTANT`   | 打开/关闭滤镜面板                        | -     | -       | -                                    | -             | -                     | -                 |
| `KEY_ID_VOLUME_UP`   | 放大变焦（1→2→3→6）                      | -     | -       | -                                    | -             | -                     | -                 |
| `KEY_ID_VOLUME_DOWN` | 缩小变焦（6→3→2→1）                      | -     | -       | -                                    | -             | -                     | -                 |
| `KEY_ID_FOCUS`       | -                                        | -     | -       | -                                    | -             | -                     | -                 |
| `KEY_ID_CAMERA`      | 开始/停止录像；滤镜面板打开时先关闭面板  | -     | -       | -                                    | -             | -                     | -                 |
| `KEY_ID_MODE`        | 切换到拍照模式；滤镜面板打开时先关闭面板 | -     | -       | -                                    | -             | -                     | -                 |
| `KEY_ID_MENU`        | 打开录像设置页；滤镜面板打开时先关闭面板 | -     | -       | 返回上一页；滤镜面板打开时先关闭面板 | -             | -                     | -                 |
| `KEY_ID_UP`          | -                                        | -     | -       | -                                    | -             | -                     | -                 |
| `KEY_ID_DOWN`        | -                                        | -     | -       | -                                    | -             | -                     | -                 |
| `KEY_ID_LEFT`        | 滤镜面板打开时选择上一个滤镜并应用       | -     | -       | -                                    | -             | -                     | -                 |
| `KEY_ID_RIGHT`       | 滤镜面板打开时选择下一个滤镜并应用       | -     | -       | -                                    | -             | -                     | -                 |
| `KEY_ID_OK`          | -                                        | -     | -       | -                                    | -             | -                     | -                 |

补充：
1. 录像页滤镜面板行为与拍照页一致：`ASSISTANT Click` 打开/关闭滤镜面板。
2. 滤镜面板打开后，除 `LEFT/RIGHT Click` 外的其他物理键事件优先关闭面板，不继续执行页面动作。
3. `LEFT/RIGHT Click` 在滤镜面板打开时切换滤镜选项，并自动应用滤镜。
4. `VOLUME_UP/VOLUME_DOWN Click` 执行变焦；底层不再上报 `ZOOM_IN/ZOOM_OUT`。
5. `KEY_ID_VOLUME_UP/DOWN` 不应由 `key_manager` 注册默认音量逻辑；录像页需要自行注册对应事件。

### 录像设置 (`page_video_settings`)

| 按键                 | Click                    | Press | Release | Long Press | Long Press 3S | Long Press 3S Release | Long Press Repeat |
| -------------------- | ------------------------ | ----- | ------- | ---------- | ------------- | --------------------- | ----------------- |
| `KEY_ID_POWER`       | -                        | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_ASSISTANT`   | -                        | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_UP`   | -                        | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_DOWN` | -                        | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_FOCUS`       | -                        | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_CAMERA`      | -                        | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MODE`        | -                        | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MENU`        | 返回上一页；滚轮弹窗打开时关闭弹窗              | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_UP`          | 上移选中；首尾循环；滚轮弹窗打开时选择上一项    | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_DOWN`        | 下移选中；首尾循环；滚轮弹窗打开时选择下一项    | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_LEFT`        | -                                               | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_RIGHT`       | -                                               | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_OK`          | 打开/确认当前选中项      | -     | -       | -          | -             | -                     | -                 |

### AI拍照页 (`page_ai_photo`)

| 按键                 | Click                              | Press                | Release                  | Long Press | Long Press 3S              | Long Press 3S Release | Long Press Repeat |
| -------------------- | ---------------------------------- | -------------------- | ------------------------ | ---------- | -------------------------- | --------------------- | ----------------- |
| `KEY_ID_POWER`       | -                                  | -                    | -                        | -          | -                          | -                     | -                 |
| `KEY_ID_ASSISTANT`   | 打开/关闭滤镜面板                   | -                    | -                        | -          | -                          | -                     | -                 |
| `KEY_ID_VOLUME_UP`   | 放大变焦（1→2→3→6）                | -                    | -                        | -          | -                          | -                     | -                 |
| `KEY_ID_VOLUME_DOWN` | 缩小变焦（6→3→2→1）                | -                    | -                        | -          | -                          | -                     | -                 |
| `KEY_ID_FOCUS`       | -                                  | 对焦一次并显示对焦框 | 重新开启 AF 并隐藏对焦框 | -          | 关闭 AF 并显示锁定态对焦框 | -                     | -                 |
| `KEY_ID_CAMERA`      | AI 拍照                            | -                    | -                        | -          | -                          | -                     | -                 |
| `KEY_ID_MODE`        | -                                  | -                    | -                        | -          | -                          | -                     | -                 |
| `KEY_ID_MENU`        | 打开 AI 拍照设置                   | -                    | -                        | 返回上一页 | -                          | -                     | -                 |
| `KEY_ID_UP`          | -                                  | -                    | -                        | -          | -                          | -                     | -                 |
| `KEY_ID_DOWN`        | -                                  | -                    | -                        | -          | -                          | -                     | -                 |
| `KEY_ID_LEFT`        | 滤镜面板打开时选择上一个滤镜并应用 | -                    | -                        | -          | -                          | -                     | -                 |
| `KEY_ID_RIGHT`       | 滤镜面板打开时选择下一个滤镜并应用 | -                    | -                        | -          | -                          | -                     | -                 |
| `KEY_ID_OK`          | -                                  | -                    | -                        | -          | -                          | -                     | -                 |

### AI拍照设置 (`page_ai_photo_settings`)

| 按键                 | Click            | Press | Release | Long Press | Long Press 3S | Long Press 3S Release | Long Press Repeat |
| -------------------- | ---------------- | ----- | ------- | ---------- | ------------- | --------------------- | ----------------- |
| `KEY_ID_POWER`       | -                | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_ASSISTANT`   | -                | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_UP`   | -                | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_DOWN` | -                | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_FOCUS`       | -                | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_CAMERA`      | -                | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MODE`        | -                | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MENU`        | 返回上一页       | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_UP`          | 上移选中；首尾循环 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_DOWN`        | 下移选中；首尾循环 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_LEFT`        | -                | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_RIGHT`       | -                | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_OK`          | 选中当前项并返回 | -     | -       | -          | -             | -                     | -                 |

### AI识别预览 (`page_ai_recognition_preview`)

| 按键                 | Click        | Press | Release | Long Press | Long Press 3S | Long Press 3S Release | Long Press Repeat |
| -------------------- | ------------ | ----- | ------- | ---------- | ------------- | --------------------- | ----------------- |
| `KEY_ID_POWER`       | -            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_ASSISTANT`   | 切换识别模式 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_UP`   | -            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_DOWN` | -            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_FOCUS`       | -            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_CAMERA`      | -            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MODE`        | -            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MENU`        | 返回上一页   | -     | -       | 返回上一页 | -             | -                     | -                 |
| `KEY_ID_UP`          | -            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_DOWN`        | -            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_LEFT`        | -            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_RIGHT`       | -            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_OK`          | -            | -     | -       | -          | -             | -                     | -                 |

### AI风格预览 (`page_ai_style_preview`)

| 按键                 | Click              | Press | Release | Long Press         | Long Press 3S | Long Press 3S Release | Long Press Repeat |
| -------------------- | ------------------ | ----- | ------- | ------------------ | ------------- | --------------------- | ----------------- |
| `KEY_ID_POWER`       | -                  | -     | -       | -                  | -             | -                     | -                 |
| `KEY_ID_ASSISTANT`   | 应用当前风格并返回 | -     | -       | -                  | -             | -                     | -                 |
| `KEY_ID_VOLUME_UP`   | -                  | -     | -       | -                  | -             | -                     | -                 |
| `KEY_ID_VOLUME_DOWN` | -                  | -     | -       | -                  | -             | -                     | -                 |
| `KEY_ID_FOCUS`       | -                  | -     | -       | -                  | -             | -                     | -                 |
| `KEY_ID_CAMERA`      | -                  | -     | -       | -                  | -             | -                     | -                 |
| `KEY_ID_MODE`        | -                  | -     | -       | -                  | -             | -                     | -                 |
| `KEY_ID_MENU`        | 返回上一页（取消） | -     | -       | 返回上一页（取消） | -             | -                     | -                 |
| `KEY_ID_UP`          | -                  | -     | -       | -                  | -             | -                     | -                 |
| `KEY_ID_DOWN`        | -                  | -     | -       | -                  | -             | -                     | -                 |
| `KEY_ID_LEFT`        | 上一风格           | -     | -       | -                  | -             | -                     | -                 |
| `KEY_ID_RIGHT`       | 下一风格           | -     | -       | -                  | -             | -                     | -                 |
| `KEY_ID_OK`          | 应用当前风格并返回 | -     | -       | -                  | -             | -                     | -                 |

### 相册 (`page_album`)

| 按键                 | Click                        | Press | Release | Long Press | Long Press 3S | Long Press 3S Release | Long Press Repeat |
| -------------------- | ---------------------------- | ----- | ------- | ---------- | ------------- | --------------------- | ----------------- |
| `KEY_ID_POWER`       | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_ASSISTANT`   | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_UP`   | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_DOWN` | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_FOCUS`       | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_CAMERA`      | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MODE`        | 切换到视频相册               | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MENU`        | 返回上一页                   | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_UP`          | 上移光标并自动滚动到可见范围 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_DOWN`        | 下移光标并自动滚动到可见范围 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_LEFT`        | 左移光标并自动滚动到可见范围 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_RIGHT`       | 右移光标并自动滚动到可见范围 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_OK`          | 预览当前光标照片             | -     | -       | -          | -             | -                     | -                 |

补充：
1. 当前光标照片使用红色 `2px` 边框标识。

### 视频相册 (`page_video_album`)

| 按键                 | Click                        | Press | Release | Long Press | Long Press 3S | Long Press 3S Release | Long Press Repeat |
| -------------------- | ---------------------------- | ----- | ------- | ---------- | ------------- | --------------------- | ----------------- |
| `KEY_ID_POWER`       | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_ASSISTANT`   | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_UP`   | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_DOWN` | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_FOCUS`       | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_CAMERA`      | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MODE`        | 切换到照片相册               | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MENU`        | 返回上一页                   | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_UP`          | 上移光标并自动滚动到可见范围 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_DOWN`        | 下移光标并自动滚动到可见范围 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_LEFT`        | 左移光标并自动滚动到可见范围 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_RIGHT`       | 右移光标并自动滚动到可见范围 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_OK`          | 预览当前光标视频             | -     | -       | -          | -             | -                     | -                 |

补充：
1. 当前光标视频使用红色 `2px` 边框标识。

### 照片预览 (`page_photo_preview`)

| 按键                 | Click          | Press | Release | Long Press | Long Press 3S | Long Press 3S Release | Long Press Repeat |
| -------------------- | -------------- | ----- | ------- | ---------- | ------------- | --------------------- | ----------------- |
| `KEY_ID_POWER`       | -              | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_ASSISTANT`   | -              | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_UP`   | -              | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_DOWN` | -              | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_FOCUS`       | -              | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_CAMERA`      | -              | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MODE`        | -              | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MENU`        | 返回上一页     | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_UP`          | -              | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_DOWN`        | -              | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_LEFT`        | 显示上一张照片 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_RIGHT`       | 显示下一张照片 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_OK`          | -              | -     | -       | -          | -             | -                     | -                 |

### 视频预览 (`page_video_preview`)

| 按键                 | Click          | Press | Release | Long Press | Long Press 3S | Long Press 3S Release | Long Press Repeat |
| -------------------- | -------------- | ----- | ------- | ---------- | ------------- | --------------------- | ----------------- |
| `KEY_ID_POWER`       | -              | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_ASSISTANT`   | -              | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_UP`   | -              | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_DOWN` | -              | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_FOCUS`       | -              | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_CAMERA`      | -              | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MODE`        | -              | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MENU`        | 返回上一页     | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_UP`          | -              | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_DOWN`        | -              | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_LEFT`        | 显示上一段视频 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_RIGHT`       | 显示下一段视频 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_OK`          | 开始/暂停播放  | -     | -       | -          | -             | -                     | -                 |

### 系统设置 (`page_system_settings`)

| 按键                 | Click                        | Press | Release | Long Press | Long Press 3S | Long Press 3S Release | Long Press Repeat |
| -------------------- | ---------------------------- | ----- | ------- | ---------- | ------------- | --------------------- | ----------------- |
| `KEY_ID_POWER`       | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_ASSISTANT`   | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_UP`   | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_DOWN` | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_FOCUS`       | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_CAMERA`      | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MODE`        | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MENU`        | 返回上一页                   | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_UP`          | 上移选中；首尾循环；自动滚动到可见范围 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_DOWN`        | 下移选中；首尾循环；自动滚动到可见范围 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_LEFT`        | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_RIGHT`       | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_OK`          | 进入/确认当前选中项          | -     | -       | -          | -             | -                     | -                 |

### AI对话 (`page_chat`)

| 按键                 | Click      | Press | Release | Long Press | Long Press 3S | Long Press 3S Release | Long Press Repeat |
| -------------------- | ---------- | ----- | ------- | ---------- | ------------- | --------------------- | ----------------- |
| `KEY_ID_POWER`       | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_ASSISTANT`   | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_UP`   | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_DOWN` | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_FOCUS`       | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_CAMERA`      | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MODE`        | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MENU`        | 返回上一页 | -     | -       | 返回上一页 | -             | -                     | -                 |
| `KEY_ID_UP`          | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_DOWN`        | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_LEFT`        | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_RIGHT`       | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_OK`          | -          | -     | -       | -          | -             | -                     | -                 |

### 版本信息 (`page_version_info`)

| 按键                 | Click      | Press | Release | Long Press | Long Press 3S | Long Press 3S Release | Long Press Repeat |
| -------------------- | ---------- | ----- | ------- | ---------- | ------------- | --------------------- | ----------------- |
| `KEY_ID_POWER`       | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_ASSISTANT`   | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_UP`   | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_DOWN` | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_FOCUS`       | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_CAMERA`      | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MODE`        | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MENU`        | 返回上一页 | -     | -       | 返回上一页 | -             | -                     | -                 |
| `KEY_ID_UP`          | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_DOWN`        | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_LEFT`        | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_RIGHT`       | -          | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_OK`          | -          | -     | -       | -          | -             | -                     | -                 |

### WiFi列表 (`page_wifi_list`)

| 按键                 | Click                        | Press | Release | Long Press | Long Press 3S | Long Press 3S Release | Long Press Repeat |
| -------------------- | ---------------------------- | ----- | ------- | ---------- | ------------- | --------------------- | ----------------- |
| `KEY_ID_POWER`       | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_ASSISTANT`   | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_UP`   | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_VOLUME_DOWN` | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_FOCUS`       | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_CAMERA`      | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MODE`        | -                            | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_MENU`        | 返回上一页（弹窗时关闭弹窗） | -     | -       | 返回上一页（弹窗时关闭弹窗） | -             | -                     | -                 |
| `KEY_ID_UP`          | 上移选中；首尾循环；自动滚动到可见范围 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_DOWN`        | 下移选中；首尾循环；自动滚动到可见范围 | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_LEFT`        | -（弹窗时见下方说明）        | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_RIGHT`       | -（弹窗时见下方说明）        | -     | -       | -          | -             | -                     | -                 |
| `KEY_ID_OK`          | 连接/断开选中 WiFi；加密 WiFi 弹出密码输入弹窗 | -     | -       | -          | -             | -                     | -                 |

补充——密码输入弹窗按键行为：

弹窗可见时，所有方向键和 OK 键均被拦截，不再控制后方 WiFi 列表。焦点在 3 个区域间切换：**键盘区域**（默认）、**取消按钮**、**确认按钮**。

| 按键 | 键盘区域 | 取消/确认按钮区域 |
| ---- | -------- | ----------------- |
| UP   | 键盘内向上移动光标；若已在第一行则切换到取消按钮 | -（无动作） |
| DOWN | 键盘内向下移动光标 | 切换回键盘区域 |
| LEFT | 键盘内向左移动光标 | 从确认切换到取消 |
| RIGHT| 键盘内向右移动光标 | 从取消切换到确认 |
| OK   | 输入当前选中的键盘字符 | 取消按钮：关闭弹窗；确认按钮：提交密码并连接 |
| MENU | 关闭弹窗 | 关闭弹窗 |

