
## 图标获取

- 推荐使用 ai 生成 svg 图片，再转为png，可以使用 claude 这类命令行工具。
- https://icons.getbootstrap.com/
- https://www.flaticon.com/
- https://www.jyshare.com/more/svgeditor/ 在线编辑 svg 图标

## 图标处理

```bash
# ImageMagick 批量图片缩放
mogrify -resize 45x45 *.png

# svg 转换为45x45的png
inkscape input.svg -w 45 -h 45 -o output.png
```
