# mmap
参考文章[https://blog.csdn.net/bbzhaohui/article/details/81665370](https://blog.csdn.net/bbzhaohui/article/details/81665370)
## 一、mmap的基本概念
&emsp;&emsp;mmap是一种内存映射文件的方法，即将磁盘上的内容映射到进程的虚拟地址空间上来，这样我们就可以通过指针来操作这一块数据，而不必用open和write来读写这段数据。并且，我们在进程空间对该段数据的操作，会被自动同步到磁盘上，这样就是进程间通信共享内存的实现原理，在一个进程里可以直接操作磁盘上某段数据，如果两个或者多个进程共享该数据段，那么另一个进程就可以得到该进程对共享数据的修改。
&emsp;&emsp;linux内核使用vm_area_struct结构来表示一个独立的虚拟内存区域，由于每个不同质的虚拟内存区域功能和内部机制都不同，因此一个进程使用多个vm_area_struct结构来分别表示不同类型的虚拟内存区域。各个vm_area_struct结构使用链表或者树形结构链接，方便进程快速访问；

## 二、mmap内存映射原理
mmap内存映射的实现过程，总的来说可以分为三个阶段：

- **1.进程启动映射过程，并在虚拟地址空间中为映射创建虚拟映射区域**
	- 进程在用户空间调用库函数mmap，原型：void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
	- 在当前进程的虚拟地址空间中，寻找一段空闲的满足要求的连续的虚拟地址
	- 为此虚拟区分配一个vm_area_struct结构，接着对这个结构的各个域进行了初始化
	- 将新建的虚拟区结构（vm_area_struct）插入进程的虚拟地址区域链表或树中
- **2.调用内核空间的系统调用函数mmap（不同于用户空间函数），实现文件物理地址和进程虚拟地址的一一映射关系**
	- 为映射分配了新的虚拟地址区域后，通过待映射的文件指针，在文件描述符表中找到对应的文件描述符，通过文件描述符，链接到内核“已打开文件集”中该文件的文件结构体（struct file），每个文件结构体维护着和这个已打开文件相关各项信息。
	- 通过该文件的文件结构体，链接到file_operations模块，调用内核函数mmap，其原型为：int mmap(struct file *filp, struct vm_area_struct *vma)，不同于用户空间库函数。
	- 内核mmap函数通过虚拟文件系统inode模块定位到文件磁盘物理地址。
	- 通过remap_pfn_range函数建立页表，即实现了文件地址和虚拟地址区域的映射关系。此时，这片虚拟地址并没有任何数据关联到主存中。
- **3.进程发起对这片映射空间的访问，引发缺页异常，实现文件内容到物理内存（主存）的拷贝**
*注：前两个阶段仅在于创建虚拟区间并完成地址映射，但是并没有将任何文件数据的拷贝至主存。真正的文件读取是当进程发起读或写操作时。*
	- 进程的读或写操作访问虚拟地址空间这一段映射地址，通过查询页表，发现这一段地址并不在物理页面上。因为目前只建立了地址映射，真正的硬盘数据还没有拷贝到内存中，因此引发缺页异常。
	- 缺页异常进行一系列判断，确定无非法操作后，内核发起请求调页过程。
	- 调页过程先在交换缓存空间（swap cache）中寻找需要访问的内存页，如果没有则调用nopage函数把所缺的页从磁盘装入到主存中。
	- 之后进程即可对这片主存进行读或者写的操作，如果写操作改变了其内容，一定时间后系统会自动回写脏页面到对应磁盘地址，也即完成了写入到文件的过程。  

	*注：修改过的脏页面并不会立即更新回文件中，而是有一段时间的延迟，可以调用msync()来强制同步, 这样所写的内容就能立即保存到文件里了。*
## 三、mmap和常规文件操作的区别
&emsp;&emsp;对linux文件系统不了解的朋友，请参阅博文《[从内核文件系统看文件读写过程](https://www.cnblogs.com/huxiao-tee/p/4657851.html)》，我们首先简单的回顾一下常规文件系统操作（调用read/fread等类函数）中，函数的调用过程：
- 进程发起读文件请求。
- 内核通过查找进程文件符表，定位到内核已打开文件集上的文件信息，从而找到此文件的inode。
- inode在address_space上查找要请求的文件页是否已经缓存在页缓存中。如果存在，则直接返回这片文件页的内容。
- 如果不存在，则通过inode定位到文件磁盘地址，将数据从磁盘复制到页缓存。之后再次发起读页面过程，进而将页缓存中的数据发给用户进程。

&emsp;&emsp;总结来说，常规文件操作为了提高读写效率和保护磁盘，使用了页缓存机制。这样造成读文件时需要先将文件页从磁盘拷贝到页缓存中，由于页缓存处在内核空间，不能被用户进程直接寻址，所以还需要将页缓存中数据页再次拷贝到内存对应的用户空间中。这样，通过了两次数据拷贝过程，才能完成进程对文件内容的获取任务。写操作也是一样，待写入的buffer在内核空间不能直接访问，必须要先拷贝至内核空间对应的主存，再写回磁盘中（延迟写回），也是需要两次数据拷贝。
&emsp;&emsp;而使用mmap操作文件中，创建新的虚拟内存区域和建立文件磁盘地址和虚拟内存区域映射这两步，没有任何文件拷贝操作。而之后访问数据时发现内存中并无数据而发起的缺页异常过程，可以通过已经建立好的映射关系，只使用一次数据拷贝，就从磁盘中将数据传入内存的用户空间中，供进程使用。
&emsp;&emsp;总而言之，常规文件操作需要从磁盘到页缓存再到用户主存的两次数据拷贝。而mmap操控文件，只需要从磁盘到用户主存的一次数据拷贝过程。说白了，mmap的关键点是实现了用户空间和内核空间的数据直接交互而省去了空间不同数据不通的繁琐过程。因此mmap效率更高。

## 四、mmap相关函数
```cpp
void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
```
**返回说明**
&emsp;&emsp;成功执行时，mmap()返回被映射区的指针。失败时，mmap()返回MAP_FAILED[其值为(void *)-1]， error被设为以下的某个值：

> EACCES：访问出错 EAGAIN：文件已被锁定，或者太多的内存已被锁定 
> EBADF：fd不是有效的文件描述词
> EINVAL：一个或者多个参数无效 
> ENFILE：已达到系统对打开文件的限制 
> ENODEV：指定文件所在的文件系统不支持内存映射
> ENOMEM：内存不足，或者进程已超出最大内存映射数量 
> EPERM：权能不足，操作不允许
> ETXTBSY：已写的方式打开文件，同时指定MAP_DENYWRITE标志 
> SIGSEGV：试着向只读区写入
> SIGBUS：试着访问不属于进程的内存区

**参数**
&emsp;&emsp;start：映射区的开始地址
&emsp;&emsp;length：映射区的长度
&emsp;&emsp;prot：期望的内存保护标志，不能与文件的打开模式冲突。是以下的某个值，可以通过or运算合理地组合在一起
> PROT_EXEC：页内容可以被执行
> PROT_READ：页内容可以被读取
> PROT_WRITE：页可以被写入
> PROT_NONE：页不可访问

&emsp;&emsp;flags：指定映射对象的类型，映射选项和映射页是否可以共享。它的值可以是一个或者多个以下位的组合体

> MAP_FIXED
> //使用指定的映射起始地址，如果由start和len参数指定的内存区重叠于现存的映射空间，重叠部分将会被丢弃。如果指定的起始地址不可用，操作将会失败。并且起始地址必须落在页的边界上。
> MAP_SHARED
> //与其它所有映射这个对象的进程共享映射空间。对共享区的写入，相当于输出到文件。直到msync()或者munmap()被调用，文件实际上不会被更新。
> MAP_PRIVATE //建立一个写入时拷贝的私有映射。内存区域的写入不会影响到原文件。这个标志和以上标志是互斥的，只能使用其中一个。
> MAP_DENYWRITE //这个标志被忽略。 MAP_EXECUTABLE //同上 MAP_NORESERVE
> //不要为这个映射保留交换空间。当交换空间被保留，对映射区修改的可能会得到保证。当交换空间不被保留，同时内存不足，对映射区的修改会引起段违例信号。
> MAP_LOCKED //锁定映射区的页面，从而防止页面被交换出内存。 MAP_GROWSDOWN
> //用于堆栈，告诉内核VM系统，映射区可以向下扩展。 MAP_ANONYMOUS //匿名映射，映射区不与任何文件关联。 MAP_ANON
> //MAP_ANONYMOUS的别称，不再被使用。 MAP_FILE //兼容标志，被忽略。 MAP_32BIT
> //将映射区放在进程地址空间的低2GB，MAP_FIXED指定时会被忽略。当前这个标志只在x86-64平台上得到支持。
> MAP_POPULATE //为文件映射通过预读的方式准备好页表。随后对映射区的访问不会被页违例阻塞。 MAP_NONBLOCK
> //仅和MAP_POPULATE一起使用时才有意义。不执行预读，只为已存在于内存中的页面建立页表入口。

&emsp;&emsp;fd：有效的文件描述词。如果MAP_ANONYMOUS被设定，为了兼容问题，其值应为-1
&emsp;&emsp;offset：被映射对象内容的起点

**相关函数**
```cpp
int munmap( void * addr, size_t len ) 
```
&emsp;&emsp;成功执行时，munmap()返回0。失败时，munmap返回-1，error返回标志和mmap一致；该调用在进程地址空间中解除一个映射关系，addr是调用mmap()时返回的地址，len是映射区的大小；当映射关系解除后，对原来映射地址的访问将导致段错误发生。

```cpp
int msync( void *addr, size_t len, int flags )
```
&emsp;&emsp;一般说来，进程在映射空间的对共享内容的改变并不直接写回到磁盘文件中，往往在调用munmap()后才执行该操作。可以通过调用msync()实现磁盘上文件内容与共享内存区的内容一致。
## 五、demo
```cpp
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
 
int main(int argc, char *argv[])
{
    int fd = 0;
    char *ptr = NULL;
    struct stat buf = {0};
 
    if (argc < 2)
    {
        printf("please enter a file!\n");
        return -1;
    }
 
    if ((fd = open(argv[1], O_RDWR)) < 0)
    {
        printf("open file error\n");
        return -1;
    }
 
    if (fstat(fd, &buf) < 0)
    {
        printf("get file state error:%d\n", errno);
        close(fd);
        return -1;
    }
 
    ptr = (char *)mmap(NULL, buf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED)
    {
        printf("mmap failed\n");
        close(fd);
        return -1;
    }
 
    close(fd);
 
    printf("length of the file is : %d\n", buf.st_size);
    printf("the %s content is : %s\n", argv[1], ptr);
 
    ptr[3] = 'a';
    printf("the %s new content is : %s\n", argv[1], ptr);
    munmap(ptr, buf.st_size);
    return 0;
}
```
