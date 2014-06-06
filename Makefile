# Redis Makefile
# Copyright (C) 2009 Salvatore Sanfilippo <antirez at gmail dot com>
# This file is released under the BSD license, see the COPYING file
#此条语句是Makefile中的条件赋值
# -g表示产生调试信息
# -rdynamic用来通知链接器将所有的符号添加到动态符号表中
# -ggdb表未要让gcc为GDB生成专用的更为丰富的调试信息 
DEBUG?= -g -rdynamic -ggdb
# -std=c99表示C语言的版本采用的是C99标准
# -pedantic打开完全服从ANSI C标准所需的全部警告诊断;拒绝接受采用了被禁止的语法扩展的程序
# -O2表示程序代码的优化级别
# -Wall表示打开警告开关
# -W不生成任何警告信息
CFLAGS?= -std=c99 -pedantic -O2 -Wall -W
# CC在Makefile中表示的是编译器，这里就是编译器的选项
CCOPT= $(CFLAGS)
# 这些OBJ基本上都是服务器端的
OBJ = adlist.o ae.o anet.o dict.o redis.o sds.o zmalloc.o lzf_c.o lzf_d.o pqsort.o
# 与性能测试相关的
BENCHOBJ = ae.o anet.o benchmark.o sds.o adlist.o zmalloc.o
# 这些OBJ基本上都是客户端的
CLIOBJ = anet.o sds.o adlist.o redis-cli.o zmalloc.o
# 服务器端
PRGNAME = redis-server
# 性能测试相关
BENCHPRGNAME = redis-benchmark
# 客户端
CLIPRGNAME = redis-cli

# 伪目标,make会将第一个出现的目标作为默认目标，就是只执行make不加目标名的时候，第一个目标名通常是all
all: redis-server redis-benchmark redis-cli

# Deps (use make dep to generate this)
# 下面是各种依赖
adlist.o: adlist.c adlist.h zmalloc.h
ae.o: ae.c ae.h zmalloc.h
anet.o: anet.c fmacros.h anet.h
benchmark.o: benchmark.c fmacros.h ae.h anet.h sds.h adlist.h zmalloc.h
dict.o: dict.c fmacros.h dict.h zmalloc.h
lzf_c.o: lzf_c.c lzfP.h
lzf_d.o: lzf_d.c lzfP.h
pqsort.o: pqsort.c
redis-cli.o: redis-cli.c fmacros.h anet.h sds.h adlist.h zmalloc.h
redis.o: redis.c fmacros.h ae.h sds.h anet.h dict.h adlist.h zmalloc.h lzf.h pqsort.h config.h
sds.o: sds.c sds.h zmalloc.h
zmalloc.o: zmalloc.c config.h

# $(OBJ)表示要生成redis-server需要依赖的文件
redis-server: $(OBJ)
	$(CC) -o $(PRGNAME) $(CCOPT) $(DEBUG) $(OBJ)
	@echo ""
	@echo "Hint: To run the test-redis.tcl script is a good idea."
	@echo "Launch the redis server with ./redis-server, then in another"
	@echo "terminal window enter this directory and run 'make test'."
	@echo ""
# 编译生成性能测试工具，$(BENCHOBJ)表示生成性能测试工具时依赖的文件 
redis-benchmark: $(BENCHOBJ)
	$(CC) -o $(BENCHPRGNAME) $(CCOPT) $(DEBUG) $(BENCHOBJ)
# 编译生成redis客户端程序
redis-cli: $(CLIOBJ)
	$(CC) -o $(CLIPRGNAME) $(CCOPT) $(DEBUG) $(CLIOBJ)
# 其实和%o:%c等价,是Makefile里的旧格式
# gcc -o test.o test.c
# 在该规则的作用下，会变成gcc -c $(CCOPT) $(DEBUG) $(COMPILE_TIME) test.c
# -c选项表示是只编译不链接
.c.o:
	$(CC) -c $(CCOPT) $(DEBUG) $(COMPILE_TIME) $<
# 删除生成的目标程序以及所有的中间目标文件
clean:
	rm -rf $(PRGNAME) $(BENCHPRGNAME) $(CLIPRGNAME) *.o
# -MM选项表示的是是列出源文件对其他文件的依赖关系 
dep:
	$(CC) -MM *.c
# 开启对服务器端进行测试
test:
	tclsh test-redis.tcl
# 开启性能测试
bench:
	./redis-benchmark
# 将工程的更新日志信息输出到本地的Changelog里
log:
	git log '--pretty=format:%ad %s' --date=short > Changelog
