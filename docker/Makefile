TAG     := trex2
VERSION := 2018_05_31
ROOTDIR  := $(abspath ..)
ifeq ($(OS),Windows_NT)
	VOLUME   := /$(subst :,,$(ROOTDIR))
else
	VOLUME   := $(ROOTDIR)
endif

all:
	docker build -t $(TAG):$(VERSION) .

shell:
	docker run --tmpfs /run --tmpfs /tmp -w $(VOLUME) -i -a stdin -a stdout -v $(VOLUME):$(VOLUME) -t $(TAG):$(VERSION)
	
