target:
	#生成静态库
	make -C ./static/ctrl/
	make -C ./static/get_and_deal_data/
	make -C ./static/log/
	make -C ./static/parse_configure_file/
	make -C ./static/queue/
	make -C ./static/signal_deal/
	make -C ./static/write_disk/
	#生成可执行程序
	make -C ./src/

install:
	#将可执行文件拷贝到/bin目录下
	cp ./src/StoreEngine ./bin
uninstall:
	#卸载
	rm -rf ./bin/StoreEngine

.PHONY:clean
clean:
	make clean -C ./static/ctrl/
	make clean -C ./static/get_and_deal_data/
	make clean -C ./static/log/
	make clean -C ./static/parse_configure_file/
	make clean -C ./static/queue/
	make clean -C ./static/signal_deal/
	make clean -C ./static/write_disk/
	make clean -C ./src/
