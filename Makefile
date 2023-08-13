CC = gcc
all: watcher

watcher: watcher.c
	$(CC) -o watcher watcher.c -lsystemd -lbsd

run: watcher
	./watcher $(ARGS)

clean: watcher
	rm -f watcher
	rm -f test

install: watcher
	cp watcher /usr/local/bin/scriptwatch
	cp scriptwatch.service /etc/systemd/system/scriptwatch.service
	systemctl enable scriptwatch.service
	systemctl start scriptwatch.service

uninstall:
	rm -f /usr/local/bin/scriptwatch
	systemctl stop scriptwatch.service
	systemctl disable scriptwatch.service
	rm -f /etc/systemd/system/scriptwatch.service
	systemctl daemon-reload

test: test.c
	$(CC) -o test test.c -lbsd
	./test