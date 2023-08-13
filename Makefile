CC = gcc
all: watcher

watcher: watcher.c
	$(CC) -o build/release/watcher src/watcher.c -lsystemd -lbsd

run: watcher
	build/release/watcher

clean: watcher
	rm -f build/*/*
	rm -f test

install: watcher
	cp build/release/watcher /usr/local/bin/scriptwatch
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