# 字体选型建议（免费商用 + 多语言）

最推荐的免费商用、支持多语言（中日韩/CJK + 拉丁文/拉丁字母）字体系列：

- 思源系列（黑体与宋体）
- 阿里巴巴普惠体
- MiSans（小米）
- OPPO Sans
- HarmonyOS Sans（华为鸿蒙字体）

这些字体覆盖语言广、设计现代、字重丰富，适合跨国项目与电子屏幕显示。

## 核心推荐与语言支持

1. 思源黑体 (Source Han Sans) / 思源宋体 (Source Han Serif)  
Google 与 Adobe 合作开发，是多语言设计的“标准件”。  
全面覆盖简体、繁体、日文、韩文，以及拉丁文、希腊文、西里尔文。

2. 阿里巴巴普惠体 (Alibaba PuHuiTi)  
阿里出品的专业字库，支持 178 个语种，覆盖欧美、东南亚等大多数地区，面向全球商业场景设计。

3. MiSans（小米澎湃 OS）  
覆盖 600+ 语言，包含 20+ 书写系统，字符数量巨大，设计现代，支持免费商用。

4. OPPO Sans  
现代简约，中英文搭配和谐，支持多种语言，适合现代数字界面。

5. HarmonyOS Sans（华为鸿蒙字体）  
覆盖 5 大书写系统，支持 105 种语言，适用于多设备阅读。

## 注意事项

- 虽然上述字体通常可免费商用，但建议在设计前确认具体版本授权条款。
- 建议从官方发布渠道或可信字体平台（如 100font.com）下载，以降低授权风险。

## TTF 移除不需要语言（字体子集化）

可通过 `fonttools` 的 `pyftsubset` 对 TTF 做子集化，只保留项目实际需要字符，减小体积并移除不需要语言字形。

### 1) 安装工具

```bash
pip install fonttools brotli zopfli
```

### 2) 按 Unicode 范围保留字符（示例：拉丁）

```bash
pyftsubset input.ttf \
  --output-file=output-subset.ttf \
  --unicodes="U+0020-007E,U+00A0-00FF" \
  --layout-features='*' \
  --glyph-names --symbol-cmap --legacy-cmap --notdef-glyph --notdef-outline \
  --recommended-glyphs --name-IDs='*' --name-legacy --name-languages='*'
```

### 3) 示例：保留中文常用区 + 拉丁，去掉日文/韩文相关字符

```bash
pyftsubset input.ttf \
  --output-file=output-zh-latin.ttf \
  --unicodes="U+0020-007E,U+3000-303F,U+4E00-9FFF,U+FF00-FFEF"
```

### 4) 按实际文案精确保留（推荐）

将项目所有可能显示文本汇总到 `used_chars.txt`，然后执行：

```bash
pyftsubset input.ttf \
  --output-file=output-subset.ttf \
  --text-file=used_chars.txt
```

这种方式体积最优，同时能最大程度降低缺字风险。
