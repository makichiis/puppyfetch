.PHONY: install uninstall clean

PREFIX= /usr

puppyfetch: puppyfetch.c
	$(CC) -O3 -Wno-unused-result -Wno-unused-parameter -masm=intel $(CFLAGS) -o $@ $<

install: puppyfetch
	install -Dm 755 puppyfetch $(DESTDIR)$(PREFIX)/bin/puppyfetch

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/puppyfetch

clean:
	rm -f puppyfetch
