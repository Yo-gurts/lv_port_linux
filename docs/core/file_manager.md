# File Manager 设计文档

## 1. 目标

`file_manager` 用于给相册页提供统一的数据访问接口，UI 不直接处理文件系统细节。

当前第一阶段只覆盖照片缩略图场景：

- 获取照片总数
- 按文件名顺序获取文件列表
- 获取指定照片的缩略图路径（供 LVGL 直接显示）

---

## 2. 职责边界

`file_manager` 负责：

- 调用 `dtcf` 获取照片目录文件列表
- 缓存文件名列表并按索引访问
- 管理缩略图生成与路径返回逻辑

`file_manager` 不负责：

- LVGL 控件操作
- 页面滚动/复用池逻辑
- 删除/重命名等文件写操作（后续可扩展）

---

## 3. 架构关系

- 页面层：`src/pages/page_album.c`
- 文件层：`src/core/file_manager.c`
- 文件列表来源：`dtcf`
- 缩略图提取：`thumbnail_extractor`（可选，受宏控制）

数据流：

1. 相册页进入时调用 `file_manager_refresh_photo_list()`
2. `file_manager` 通过 `dtcf` 扫描并缓存照片文件名
3. UI 按索引调用 `file_manager_get_photo_thumbnail_path()`
4. `file_manager` 返回可被 `lv_img_set_src()` 使用的 `A:` 路径

---

## 4. API 设计

头文件：`include/core/file_manager.h`

```c
int file_manager_refresh_photo_list(void);
int file_manager_get_photo_count(void);
int file_manager_get_photo_name(int index, char* out_name, size_t out_size);
int file_manager_get_photo_thumbnail_path(int index, char* out_path, size_t out_size);
```

语义约定：

- `0` 表示成功，负值表示失败
- `index` 越界返回失败
- 输出缓冲区不足返回失败

---

## 5. 关键实现说明

### 5.1 列表获取（dtcf）

`file_manager_refresh_photo_list()` 逻辑：

- 从 `PHOTO_ALBUM_IMAGE_PATH` 解析 `main_dir` 与子目录名
- 初始化 `dtcf` 并注册 selector/compare 回调
- 调用 `DTCF_Scan()` 更新列表
- 分批调用 `DTCF_GetFileNameByInx()` 拉取文件名并缓存

筛选策略：

- 仅保留 `.jpg/.jpeg`
- 隐藏文件（`.` 开头）过滤

### 5.2 缩略图路径

`file_manager_get_photo_thumbnail_path()` 逻辑：

- 先构造目标缩略图路径：`PHOTO_ALBUM_IMAGE_THUMB_PATH + basename + .jpg`
- 若缩略图已存在且有效（大小>0），直接返回
- 若不存在：
  - 创建缩略图目录
  - 若开启 `thumbnail_extractor`，尝试从原图提取并落盘
  - 提取失败则回退返回原图路径

这样保证 UI 始终可拿到可显示路径，避免空白项。

---

## 6. 与 UI 的协作约束

相册页仅做展示：

- `total_photos` 来自 `file_manager_get_photo_count()`
- 每个 item 的图片来源来自 `file_manager_get_photo_thumbnail_path()`

不要在页面里再次实现：

- 目录扫描
- 文件排序
- 缩略图文件生成

---

## 7. 构建说明

- `dashcam` 主工程应使用 `src/core/file_manager.c`（dtcf 实现）。
- `src/mock/file_manager.c` 仅用于本地模拟构建场景，不用于主固件链路。

---

## 8. 后续扩展建议

1. 增加分页批量接口，减少高频单项查询开销。
2. 增加删除/重命名后增量更新接口，避免全量扫描。 
3. 增加视频列表和视频缩略图能力，统一照片/视频媒体访问。
