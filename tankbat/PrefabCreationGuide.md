Unity预制体创建指南
1. 坦克预制体创建步骤
苏联T-34坦克

创建一个空GameObject，命名为"T34"

添加子对象：

Body (Cube) - 坦克车身

Turret (Cube) - 炮塔

Barrel (Cube) - 炮管

LeftTracks (Cube) - 左侧履带

RightTracks (Cube) - 右侧履带

添加组件：

Rigidbody

BoxCollider

TankController脚本

设置Tag: "Tank"

拖拽到Prefabs文件夹保存为预制体

美国M4谢尔曼坦克

创建M4预制体

不同的车身形状和炮塔设计

使用蓝色材质

添加音效组件

德国虎式坦克

创建Tiger预制体

更大的尺寸和装甲

使用灰色材质

添加重型坦克特效

2. 子弹预制体
普通子弹

创建Sphere

添加Trail Renderer组件

添加Point Light组件

添加BulletController脚本

添加AudioSource组件

设置Tag: "Bullet"

穿透子弹

复制普通子弹

修改颜色为红色

修改大小为1.5倍

在BulletController中设置penetrating=true

3. 特效预制体
爆炸效果

创建Particle System

配置：

Start Lifetime: 0.5-2.0

Start Speed: 5-20

Start Size: 0.5-2.0

Color: 橙黄色渐变

Shape: Sphere

添加爆炸音效

烟雾效果

创建Particle System

配置：

Start Lifetime: 3-5

Start Speed: 1-3

Start Size: 2-5

Color: 灰色

Emission Rate: 10

护盾效果

创建Sphere

添加透明材质

添加粒子系统环绕效果

添加Hologram Shader

4. UI预制体
血条

创建Canvas

添加Image作为背景

添加Image作为血条填充

添加Text显示血量

添加HealthBar脚本

设置为Screen Space - Camera

小地图

创建Render Texture

创建小地图相机

创建小地图UI

添加玩家位置指示器

5. 材质和纹理建议
坦克材质

苏联: 暗红色金属材质

美国: 蓝色迷彩材质

德国: 灰色金属材质

环境材质

地面: 草地纹理

障碍物: 岩石纹理

天空盒: 晴朗天空

6. 音效建议
音效文件

引擎声: 低沉的柴油引擎

开火声: 炮击声

爆炸声: 重型爆炸

命中声: 金属撞击

能力音效: 科幻音效

7. 优化建议

使用对象池管理子弹和特效

使用LOD系统优化远距离坦克

使用Occlusion Culling

合并材质批次

使用GPU Instancing