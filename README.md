Redis 源码注释
======================

<a href="http://www.cnblogs.com/WJ5888/category/669887.html" target="_blank">
Redis源码注解个人博客
</a>

作者：Code研究者

目前就职:网易研发开发工程师

Email:194312815@qq.com

1.<a href="http://www.cnblogs.com/WJ5888/p/4371647.html" target="_blank">Redis内存回收:LRU算法</a>

Redis中采用两种算法进行内存回收，引用计数算法以及LRU算法，在操作系统内存管理一节中，我们都学习过LRU算法(最近最久未使用算法),那么什么是LRU算法呢,LRU算法作为内存管理的一种有效算法,其含义是在内存有限的情况下，当内存容量不足时，为了保证程序的运行，这时就不得不淘汰内存中的一些对象，释放这些对象占用的空间，那么选择淘汰哪些对象呢...

2.<a href="http://www.cnblogs.com/WJ5888/p/4516782.html" target="_blank">Redis有序集内部实现原理分析</a>

Redis中支持的数据结构比Memcached要多的多啦，如基本的字符串、哈希表、列表、集合、可排序集，在这些基本数据结构上也提供了针对该数据结构的各种操作，这也是Redis之所以流行起来的一个重要原因,当然Redis能够流行起来的原因，远远不只这一个,如支持高并发的读写、数据的持久化、高效的内存管理及淘汰机制...


3.<a href="http://www.cnblogs.com/WJ5888/p/4516782.html" target="_blank">Redis有序集内部实现原理分析(二)</a>

本篇博文紧随上篇Redis有序集内部实现原理分析，在这篇博文里凡出现源码的地方均以下述`src/version.h`中定义的Redis版本为主,在上篇博文Redis有序集内部实现原理分析中，我分析了Redis从什么时候开始支持有序集、跳表的原理、跳表的结构、跳表的查找/插入/删除的实现,理解了跳表的基本结构，理解Redis中有序集的实...

