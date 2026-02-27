# Album Design（相册页设计说明）

## 1. 目标

相册页围绕以下目标实现：

- 大量照片下保持低内存占用（虚拟列表复用 item）
- 交互与手机相册一致（连续纵向滚动）
- 支持多选/全选/删除流程
- 快速滚动条可拖拽定位
- 滚动时显示进度（`top_notice` 展示 `x/total`）

## 2. 页面结构

页面由两部分组成：

- 顶部导航栏（普通模式/选择模式切换）
- 内容区（可滚动网格 + 右侧快速滚动条）

普通模式：显示返回、拍照、录像、选择按钮。  
选择模式：显示取消、全选、已选数量、删除按钮。

## 3. 核心数据结构

见 `include/pages/page_album.h`：

- `album_layout_config_t`
  - 自动布局参数：`cols/start_x/row_height/visible_rows/pool_rows`
- `album_item_t`
  - 复用池条目：`container/img/select_box/index/photo_index/is_visible`
- `page_album_data_t`
  - 页面对象、复用池、滚动条状态缓存
  - 选择态：`selection_mode/selected_flags/selected_count`
  - 删除态：`deleting_in_progress/prev_input_block_mask`

## 4. 虚拟列表与布局策略

### 4.1 自动布局

根据可用宽高计算列数与可见行数，不写死固定页数。

### 4.2 复用池

- 只创建 `cols * (visible_rows + buffer_rows)` 个 item
- 滚动时只更新位置和内容，不创建/销毁大量对象

### 4.3 索引映射

UI 展示顺序为“新到旧”：

- `display_index`：网格中的显示序号（从 0 开始）
- `photo_index`：真实文件列表序号
- 通过 `photo_index = total_photos - 1 - display_index` 映射

## 5. 快速滚动条设计

### 5.1 外观

- 轨道隐藏（主轨/指示轨透明）
- 仅保留较宽的 knob，增强触控体验

### 5.2 同步关系

为了匹配当前竖向 slider 的视觉方向，使用反向映射：

- 滚动 -> 滑块：`slider_value = max_scroll_y - scroll_y`
- 滑块 -> 滚动：`target_y = max_scroll_y - slider_value`

### 5.3 首帧一致性

首次进入相册时，先 `scroll_to_y(0)`，再执行一次异步 deferred 同步，避免首帧布局未稳定导致滑块位置反向。

## 6. 进度提示（top_notice）

滚动列表或拖动滑块时，实时计算“最后一张可见照片序号”：

- 计算视口底部所在行
- 得到该行最后一张可见索引
- 通过 `top_notice_show_for("x/total", TOP_NOTICE_TYPE_INFO, 600)` 显示

该方案替代了右侧跟随标签，减少控件复杂度。

## 7. 选择与删除流程

### 7.1 选择模式

- 长按 item 进入选择模式
- 单击切换选中状态
- 支持“全选/取消全选”
- 顶栏显示“已选择 N 项”

### 7.2 删除流程

- 删除前启用输入屏蔽（保留电源键）
- 显示阻塞提示
- 删除后刷新文件列表、重建内容高度、退出选择模式
- 恢复输入屏蔽状态

## 8. 文件管理接口依赖

相册页依赖 `file_manager` 提供：

- `file_manager_refresh_photo_list`
- `file_manager_get_photo_count`
- `file_manager_get_photo_thumbnail_path`

mock 实现采用本地文件系统扫描（`opendir/readdir/stat`），不依赖 DTCF。

## 9. 主要文件

- 页面与交互：`src/pages/page_album.c`
- 页面数据结构：`include/pages/page_album.h`
- mock 文件管理：`src/mock/file_manager.c`

## 10. 后续可演进项

- 滑块与网格滚动加入惯性/动画同步策略
- 删除流程增加可取消与失败回滚提示
- 缩略图缓存淘汰策略（按容量/LRU）
- 分辨率切换时的布局重算与过渡优化
