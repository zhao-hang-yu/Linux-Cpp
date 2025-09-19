## Linux环境专栏

VMware中安装Ubuntu教程：https://blog.csdn.net/m0_63082093/article/details/146415053

`ifconfig`查看虚拟机IP，使用FinalShell进行连接

更换阿里云源：

`sudo vim /etc/apt/sources.list`

```
# 阿里云 Ubuntu 22.04 软件源
deb http://mirrors.aliyun.com/ubuntu/ jammy main restricted universe multiverse
deb http://mirrors.aliyun.com/ubuntu/ jammy-updates main restricted universe multiverse
deb http://mirrors.aliyun.com/ubuntu/ jammy-backports main restricted universe multiverse
deb http://mirrors.aliyun.com/ubuntu/ jammy-security main restricted universe multiverse

# 源码包
deb-src http://mirrors.aliyun.com/ubuntu/ jammy main restricted universe multiverse
deb-src http://mirrors.aliyun.com/ubuntu/ jammy-updates main restricted universe multiverse
deb-src http://mirrors.aliyun.com/ubuntu/ jammy-backports main restricted universe multiverse
deb-src http://mirrors.aliyun.com/ubuntu/ jammy-security main restricted universe multiverse
```

配置Samba：https://blog.csdn.net/qq_42417071/article/details/136328807

配置Gcc：`sudo apt-get install build-essential`



### Linux命令

#### 处理目录的命令

**ls**：列出目录

**cd**：切换目录

**pwd**：显示目前的目录

**mkdir**：创建一个新的目录

**rmdir**：删除一个空的目录

**cp**：复制文件或目录

**rm**：移出文件或目录

**mv**：移动文件与目录，或修改文件与目录名

#### 文件内容查看

**cat**：显示文件全部内容

**tac**：倒序显示文件内容

**nl**：显示内容的同时，输出行号

**more**：分页显示文件内容

**less**：支持向前翻页的分页

**head**：显示前几行内容

**tail**：显示尾几行内容
