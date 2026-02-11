# 触摸输入与滑动手势优化

## 1. GESTURE_BUBBLE 标志的作用

LVGL 的手势事件遵循**冒泡机制**，类似于 DOM 事件。当用户触摸屏幕时：

```c
// lvgl/src/indev/lv_indev.c indev_gesture() 函数
lv_obj_t * gesture_obj = indev->pointer.act_obj;

/*If gesture parent is active check recursively the gesture attribute*/
while(gesture_obj && lv_obj_has_flag(gesture_obj, LV_OBJ_FLAG_GESTURE_BUBBLE)) {
    gesture_obj = lv_obj_get_parent(gesture_obj);
}

/* 最终在 gesture_obj 上触发 LV_EVENT_GESTURE */
lv_obj_send_event(gesture_obj, LV_EVENT_GESTURE, indev_act);
```

**流程说明**：
1. 查找当前按下的对象（act_obj）
2. 如果对象有 `LV_OBJ_FLAG_GESTURE_BUBBLE` 标志，向上冒泡到父对象
3. 重复直到找到没有该标志的对象
4. 在最终对象上触发 `LV_EVENT_GESTURE`

**注意事项**：
- 默认情况下，对象**没有**这个标志
- 如果设置了 `GESTURE_BUBBLE`，事件会冒泡到父对象
- 如果父对象都没有绑定回调，事件会丢失

**正确用法**：
```c
/* 清除 GESTURE_BUBBLE，确保事件直接在当前对象上触发 */
lv_obj_clear_flag(container, LV_OBJ_FLAG_GESTURE_BUBBLE);

/* 绑定滑动手势回调 */
lv_obj_add_event_cb(container, swipe_callback, LV_EVENT_GESTURE, user_data);
```

## 2. 滑动与点击冲突处理

轻轻滑动时可能达不到滑动阈值，但释放时仍会触发点击事件。解决方案是检测到滑动后调用 `lv_indev_wait_release()` 忽略后续点击：

```c
/* 通用右滑返回回调 */
void page_manager_swipe_right_cb(lv_event_t* e)
{
    page_manager_t* pm = lv_event_get_user_data(e);
    if (!pm) return;

    if (lv_event_get_code(e) == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            MLOG_INFO("Swipe right, back");
            page_manager_back(pm);
        }
        /* 检测到滑动，忽略后续的点击事件 */
        lv_indev_t* indev = lv_indev_get_act();
        if (indev) {
            lv_indev_wait_release(indev);
        }
    }
}
```

**原理**：
- `lv_indev_wait_release()` 设置 `wait_until_release` 标志
- LVGL 在释放时会检查该标志，跳过 `CLICKED` 等事件的发送

**重要**：这是避免滑动误触的关键优化！

## 3. 滑动阈值配置

滑动识别的阈值由 `LV_INDEV_DEF_GESTURE_LIMIT` 定义，默认为 50 像素：

```c
// lvgl/src/indev/lv_indev.c
#define LV_INDEV_DEF_GESTURE_LIMIT        150  // 根据需求调整
```

可根据产品需求调整：
- 过小：容易误触发
- 过大：需要滑得更远才能识别

## 4. 触摸事件类型

| 事件类型 | 触发时机 |
|---------|---------|
| `LV_EVENT_PRESSED` | 触摸按下 |
| `LV_EVENT_RELEASED` | 触摸释放 |
| `LV_EVENT_CLICKED` | 点击（按下并释放） |
| `LV_EVENT_GESTURE` | 手势识别完成（需要滑动达到阈值） |

**常见问题**：
- 轻轻滑动 → 不触发 `GESTURE`，但触发 `CLICKED`
- 使用 `lv_indev_wait_release()` 可避免此问题

## 5. 输入设备获取

在事件回调中获取当前输入设备：

```c
lv_indev_t* indev = lv_indev_get_act();  // 获取当前活动的输入设备

/* 获取触摸坐标 */
lv_point_t point;
lv_indev_get_point(indev, &point);

/* 获取手势方向 */
lv_dir_t dir = lv_indev_get_gesture_dir(indev);
```

## 6. 推荐的事件绑定模式

```c
/* 方法1：绑定到全屏容器，统一处理滑动手势 */
lv_obj_add_event_cb(full_screen_container, swipe_cb, LV_EVENT_GESTURE, pm);

/* 方法2：screen 级别绑定（适用于全屏滑动返回） */
lv_obj_add_event_cb(lv_screen_active(), swipe_cb, LV_EVENT_GESTURE, pm);

/* 确保容器没有 GESTURE_BUBBLE 标志 */
lv_obj_clear_flag(container, LV_OBJ_FLAG_GESTURE_BUBBLE);
```

## 7. 完整示例：页面滑动返回

```c
// page_manager.c

/* 通用右滑返回回调 - 所有页面可直接作为事件回调使用
 * 当检测到滑动时，调用 lv_indev_wait_release() 避免释放时触发点击 */
void page_manager_swipe_right_cb(lv_event_t* e)
{
    page_manager_t* pm = (page_manager_t*)lv_event_get_user_data(e);
    if (!pm) {
        return;
    }
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_GESTURE) {
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        if (dir == LV_DIR_RIGHT) {
            MLOG_INFO("Swipe right, back to previous page");
            page_manager_back(pm);
        }
        /* 检测到滑动，忽略后续的点击事件 */
        lv_indev_t* indev = lv_indev_get_act();
        if (indev) {
            lv_indev_wait_release(indev);
        }
    }
}
```

```c
// page_xxx.c - 页面初始化

/* 创建页面容器 */
data->container = lv_obj_create(lv_screen_active());
lv_obj_set_size(data->container, LV_PCT(100), LV_PCT(100));

/* 清除 GESTURE_BUBBLE，确保事件直接在 container 上触发 */
lv_obj_clear_flag(data->container, LV_OBJ_FLAG_GESTURE_BUBBLE);

/* 添加滑动手势回调 */
lv_obj_add_event_cb(data->container, page_manager_swipe_right_cb, LV_EVENT_GESTURE, pm);
```

## 8. 注意事项

1. **避免在 container 上绑定 CLICKED 事件** - 子控件的点击会冒泡上来
2. **滑动和点击逻辑分离** - 滑动用 GESTURE，点击用子控件的事件回调
3. **使用 lv_indev_wait_release() 避免滑动触发点击** - 重要优化！
4. **注意 GESTURE_BUBBLE 的影响** - 确保事件冒泡到正确的对象
5. **调整合适的滑动阈值** - 根据屏幕大小和交互体验设置

## 9. 相关文件

- `src/core/page_manager.c` - 滑动回调实现
- `src/pages/*.c` - 各页面事件绑定
- `lvgl/src/indev/lv_indev.c` - LVGL 手势识别源码
