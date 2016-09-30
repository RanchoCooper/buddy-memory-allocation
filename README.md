## 前言
本来准备看下<linux/sched.h>的源码, 结果从进程描述符看到slab分配器再到这一篇关于`buddy_memory_allocation` 的实现

作为内存管理的实现方案之一, buddy的亮点在于快速搜索(基于二叉树的log(N)时间复杂度), 以及避免内存碎片(最佳适配, 但同样有内部碎片问题, 得于斯者毁于斯)

## 算法过程

其内存分配和释放的过程一张图就能说明白

![算法图示](算法图示.png)

对于要分配的内存大小N, 首先要找大于它的2的最小幂N2 (比如10字节就变成16字节), 然后寻找大小合适的内存块. 找到则返回, 没找到则做一次"切割", 继续寻找-切割寻找..

而当内存释放的时候, 如果相邻块未使用则将其合并(如果可能就还原回切割前的状态)

## 极简实现
像上面需要组织"一分为二"的结构, 我们自然会采用二叉树(实际代码里是数组形式的完全二叉树)来接管内存. 每个节点的初始值为其管理的内存单元数, 如果被分配了就值零. 比如图中初始化之后tree[0] == 16, tree[2] == 8. 而在每次更新一个节点的状态后, 同时要回溯更新父节点

![实现图示](实现图示.png)

### 分配器结构体
```c
struct buddy {
    uint32_t size;          // 管理内存的单元总数
    uint32_t tree[1];       // 实际只需数组指针, 如果是GCC完全可以写成[0], 不占用空间
};
```

### 辅助函数和宏

```c
// 是否为2的幂
#define IS_POW(x) ( !( (x) & ((x) - 1) ) )
// 二叉树相关
#define MAX(a, b) ( (a) > (b) ? (a) : (b) )
#define PARENT(index) ( ((index) + 1) >> 1 -1 )
#define LEFT_SUB(index) ( (index) << 1 + 1 )
#define RIGHT_SUB(index) ( (index) << 1 + 2 )

// 最接近的幂
static uint32_t
fixsize(uint32_t x) {
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}
```

### 分配器初始化
```c
struct buddy*
buddy_new(int size) {
    struct buddy *self;     // 分配器
    uint32_t node_size;     // 总节点数
    int i;

    if (size < 1 || !IS_POW(size))
        return NULL;

    self = (struct buddy*) malloc(2 * size * sizeof(uint32_t));     // 总节点数 == 叶子节点 * 2 - 1

    self->size = size;
    node_size = size << 1;

    for (i = 0; i < node_size - 1; ++i) {
        if (IS_POW(i + 1))          // 最左检测, 更新管理的单元数
            node_size >> 1;
        self->tree[i] = node_size;  // 同一层管理相同数量的单元
    }
    return self;
}
```

### 内存分配过程

```c
uint32_t buddy_alloc(struct buddy *self, uint32_t size) {
    uint32_t index = 0;
    uint32_t node_size;
    uint32_t offset = 0;

    if (self == NULL)
        return -1;

    if (size <= 0)
        size = 1;
    else if (!IS_POW(size))
        size = fixsize(size);       // 调整size为2的幂

    if (self->tree[index] < size)   // 请求内存过大
        return -1;

    // 深度遍历二叉树, 进行适配搜索
    for (node_size = self->size; node_size != size; node_size >> 1) {
        if (self->tree[LEFT_SUB(index)] >= size)
            index = LEFT_SUB(index);
        else
            index = RIGHT_SUB(index);
    }

    // 找粗并更新状态, 换算偏移值
    self->tree[index] = 0;
    offset = (index + 1) * node_size - self->size;

    // 回溯更新父节点为左右子树中的大值
    while (index) {
        index = PARENT(index)
        self->tree[index] = MAX(self->tree[LEFT_SUB(index)], self->tree[RIGHT_SUB(index)]);
    }

    return offset;
}
```

### 内存释放过程

```c
void buddy_free(struct buddy *self, uint32_t offset) {
    uint32_t node_size, index = 0;
    uint32_t left_node, right_node;

    assert(self && offset >= 0 && offset < self->size);

    node_size = 1;
    index = offset + self->size - 1;

    // 状态更新
    for ( ; self->tree[index]; index = PARENT(index)) {
        node_size <<= 1;
        if (index == 0)
            return;
    }

    self->tree[index] = node_size;

    // 合并相邻块
    while (index) {
        index = PARENT(index);
        node_size <<= 1;

        left_node = self->tree[LEFT_SUB(index)];
        right_node = self->tree[RIGHT_SUB(index)];

        if (left_node + right_node == node_size)
            self->tree[index] = node_size;
        else
            self->tree[index] = MAX(left_node, right_node);
    }
}
```

## 后记
第一次看云风的代码时有很多地方没看明白, Google了几篇文章之后才明白十之八九.
而写完代码再来写文章, 就都明白了

[0]: [代码位置](https://github.com/RanchoCooper/)
[1]: [云风博客](http://blog.codingnow.com/2011/12/buddy_memory_allocation.html)
[2]: [酷壳文章](http://coolshell.cn/articles/10427.html)

