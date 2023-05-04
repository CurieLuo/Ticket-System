# 火车票管理系统说明文档

> by 罗祺皓

## 项目概述

本次作业要求实现一个类似于 [12306](https://www.12306.cn/) 的火车票订票系统，该系统向用户提供购票业务相关功能，包括车票查询、购票、订单操作等，以及向管理员提供后台管理功能。

系统需要在**本地**储存用户数据、购票数据、车次数据，并对其进行高效操作。

**注：关于代码实现的更多细节，请参见代码注释**

## 文件树

![file tree](file_tree.jpg)

## 一、数据存储

### 1. vector类

文件位置：/lib/vector.hpp

sjtu::vector 类支持 std::vector 的常用接口。

### 2. Hashmap类

文件位置：/lib/Hashmap.hpp

Hashmap 类的实现为遵循LRU原则的 linked hash map, 并支持 std::unordered_map 的主要接口。

### 3. BPT类 & CachedBPT类

文件位置：/database/BPT.hpp & /database/CachedBPT.hpp 

BPT.hpp 中实现功能和接口类似于 std::map （不支持重复key）的 BPT 类，用 vector 类实现外存回收。CachedBPT 类继承 BPT 类，使用 Hashmap 类实现 LRU cache，并重写 BPT 类的外存读写函数 (虚函数)。BPT使用的存储文件存放在根目录下的/bin文件夹中。

#### (1) BPT 内部逻辑：

BPT每个节点大小约4KB，内部节点存储的key为子树key最大值，数量等于的节点分支数。叶节点不直接存储value，而是存储value在存储文件中的地址，这样叶节点和内部节点可以使用同一个类而不会浪费空间，也能够通过返回value地址 (handle) 让外部调用者直接访问value。

BPT的insert和erase采用递归写法，回溯时维护树的平衡性，对节点进行split/merge操作（其中merge操作可能只是向兄弟节点借一个元素）。

#### (2) BPT类支持的主要操作：

| 名称        | 参数       | 返回值   | 备注                  |
| ----------- | ---------- | -------- | --------------------- |
| insert      | key, value | int      | 返回handle            |
| set         | key, value | void     | 修改value             |
| erase       | key        | void     |                       |
| find        | key        | iterator | 找不到返回end()，下同 |
| lower_bound | key        | iterator | 用于连续遍历          |
| get         | key        | value    |                       |
| clear       | void       | void     |                       |

#### (3) BPT::iterator类支持的主要操作：

| 名称       | 参数  | 返回值    | 备注      |
| ---------- | ----- | --------- | --------- |
| operator++ | void  | iterator& | 前置++    |
| operator!= | void  | bool      |           |
| key        | void  | key       |           |
| value      | void  | value     |           |
| set        | value | void      | 修改value |

## 二、其他库

**注：以下文件都存放在/lib文件夹中**

### 1. String类

文件位置：String.hpp

实现一个定长的字符串类，支持 std::string 的常用接口。

### 2. Scanner类

文件位置：Scanner.hpp

实现一个类似于特化的std::istringstream的Scanner类，可以提取字符串、整数和指令参数名称（保证是单个字符，参见作业要求）。

### 3. Date类 & Time类

文件位置：datetime.hpp

实现日期和时间的运算、比较、格式转换和I/O。

## 三、主体逻辑

**注：以下文件都存放在/src文件夹中，各种信息默认用CachedBPT存储在外存中**

### 1. UserSystem类

文件位置：UserSystem.hpp

管理用户的子系统，支持 **add_user**，**login**，**logout**，**query_profile**，**modify_profile** 操作。

#### (1) UserInfo类

记录用户信息。

#### (2) login相关

用Hashmap存储用户登录信息。

### 2. TrainSystem类

文件位置：TrainSystem.hpp

管理列车的子系统，支持 **add_train**，**delete_train**，**release_train**，**query_train** 操作。

#### (1) TrainInfo类

记录列车信息。

#### (2) SeatInfo类

记录已发布车次（注：车次指某一列车在某一天的运行情况，下同）的空余座位信息。

#### (3) Passby类

记录经过某一车站的已发布列车。

### 3. TicketSystem类

文件位置：TicketSystem.hpp

继承 UserSystem 和 TrainSystem 类，并管理火车票和订单，支持 **query_ticket**，**query_transfer**，**buy_ticket**，**query_order**，**refund_ticket** ，**clean** 操作。

#### (1) Ticket类

记录车票信息，只临时存储在vector中。

#### (2) Transfer类

记录转车车票信息，只临时存储在vector中。

#### (3) Order类

记录用户的订单信息。

#### (4) Pending类

记录已发布车次的候补订单。
