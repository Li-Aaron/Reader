# Reader (aaron-mod)
A win32 txt file reader
使用说明：
[toc]

### 一、快捷键
- F12：     隐藏/显示边框
- F11:      全屏/退出全屏
- Esc:      退出全屏
- Ctrl + O：打开新文件
- Ctrl + F: 全文关键字查找，并跳转到指定位置
- Ctrl + T：置顶/取消置顶
- Ctrl + G: 全文进度百分比跳转
- Ctrl + →：快速跳转到下一章
- Ctrl + ←：快速跳转到上一章
- Ctrl + H：隐藏/显示边框
- →：       下一页
- ←：       上一页
- ↑：       向上滚动 N 行 (N: setting > config > 文本滚动速度，默认为1)
- ↓：       向下滚动 N 行 (N: setting > config > 文本滚动速度，默认为1)
- Ctrl + 鼠标滚轮：调节窗口透明度
  向上滚动：透明度变低
  向下滚动：透明度变高
- 空格键：  开始/停止自动翻页 (setting > config > 自动翻页时间间隔，默认为3000ms)
  点击菜单选项时，会自动暂停“自动翻页”，菜单完成后会自动恢复“自动翻页”
  书本翻到最后一页会自动停止“自动翻页”

### 二、热键
-  Alt + H： 快速隐藏/显示窗口 (可以在 setting > config > 热键设置，进行自定义)

### 三、翻页
-  翻页模式一：(setting > config > 鼠标左右键点击翻页)
   -  鼠标左键单击：下一页
   -  鼠标右键单击：上一页
   -  如下表

<table>
  <tr >
    <td>隐藏边框时按住可拖动</td>
  </tr>
  <tr>
    <td>左键单击上一页</td>
  </tr>
  <tr>
    <td>右键单击下一页</td>
  </tr>
</table>

-  翻页模式二：(setting > config > 鼠标左键点击左右侧翻页)
   -  鼠标左键点击界面右侧1/3处：下一页
   -  鼠标左键点击界面左侧1/3处：上一页
   -  如下表

<table>
  <tr>
    <td rowspan =3>上一页 </td>
    <td rowspan =3>隐藏边框时按住可拖动  </td>
    <td rowspan =3>下一页</td>
  </tr>
</table>

-  鼠标向上滚动：上N行 (N: setting > config > 文本滚动速度，默认为1)
-  鼠标向下滚动：下N行 (N: setting > config > 文本滚动速度，默认为1)
-  鼠标按键4：上翻页
-  鼠标按键5：下翻页

### 四、其他设置
-  setting > font：   字体设置
-  setting > color：  背景颜色设置
-  setting > Image：  背景图片设置
   - 勾选  “启用背景图片”，则会使用背景图片
   - 不勾选“启用背景图片”，则会使用setting > color设置
   - 对于不规则的小图，建议设置背景模式为“旋转平铺”，效果更佳，如包内的"skin_example.jpg"
-  setting > config： 进入设置菜单
-  setting > default：还原默认设置
-  help > proxy: 代理设置，仅用于软件版本更新检测

### 五、备注
-  如果需要还原软件最原始的状态（或者清除File菜单里面的文件列表）：
    请删除Reader.exe同级目录下面的 “cache.dat” 文件。
    该文件为隐藏文件。
    至于如何显示隐藏文件，请自行百度。
-  三种新文件打开方式：
   - Ctrl + O
   - 菜单：File > Open
   - 文件拖拽
-  边框隐藏时，鼠标左键按住页面顶部，可以进行拖动