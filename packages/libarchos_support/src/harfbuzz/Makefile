ifeq ($(TARGET), SIM)
TGT_PATH = sim
CROSS =
endif
ifeq ($(TARGET), ARM)
TGT_PATH = arm
CROSS = arm-linux-
endif

INCS = -I./src
VPATH = ./src

CC = $(CROSS)gcc
CFLAGS += $(INCS)
LD = $(CROSS)ld
OBJCOPY = $(CROSS)objcopy

CMD_CC_O_C =  $(CC) $(CFLAGS) -o $@ -c $<

SRC =  \
	harfbuzz-global.c \
	harfbuzz-buffer.c \
	harfbuzz-stream.c \
	harfbuzz-gdef.c \
	harfbuzz-gpos.c \
	harfbuzz-gsub.c \
	harfbuzz-open.c \
	harfbuzz-shaper.c \
	harfbuzz-arabic.c \
	harfbuzz-oem.c

OTHR_SRC = \
	harfbuzz-hebrew.c \
	harfbuzz-tibetan.c \
	harfbuzz-indic.c \
	harfbuzz-khmer.c \
	harfbuzz-hangul.c \
	harfbuzz-myanmar.c

OBJS = $(patsubst %,$(TGT_PATH)/%,$(SRC:.c=.o))

DEPDIR = .deps

MAKEDEPEND = $(CC) -MM $(CFLAGS) -MT $@ -o $(TGT_PATH)/$(DEPDIR)/$*.dep $<

all: $(TGT_PATH)/libharfbuzz.a

$(TGT_PATH)/%.o : %.c
	@$(MAKEDEPEND); 
	$(CMD_CC_O_C)

$(TGT_PATH)/libharfbuzz.a: $(OBJS) Makefile
	$(LD) -o $(TGT_PATH)/libharfbuzz.a -i $(OBJS)

clean:
	-rm -f sim/*.o
	-rm -f arm/*.o
	-rm -f sim/.deps/*.dep
	-rm -f arm/.deps/*.dep
	-rm -f sim/*.a
	-rm -f arm/*.a
	-rm -f .compiled

# include deps depending on target
ifneq (,$(TGT_PATH))
-include $(SRC:%.c=$(TGT_PATH)/$(DEPDIR)/%.dep)
endif

