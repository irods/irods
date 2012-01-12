# make file for driver test codes
ifndef buildDir
buildDir =      $(CURDIR)/../../..
endif

include $(buildDir)/config/config.mk
include $(buildDir)/config/platform.mk
include $(buildDir)/config/directories.mk
include $(buildDir)/config/common.mk
 
vpath %.c src

CFLAGS = $(MY_CFLAG)
LDFLAGS += $(LDADD)

TESTOBJS = hpsstest.o
TARGETS = hpsstest

all:: $(TARGETS)

clean:
	rm -f $(TARGETS) $(TESTOBJS)

$(TESTOBJS): %.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

lowlevtest: lowlevtest.o
	$(LDR) -o $@ $^ $(LDFLAGS)

