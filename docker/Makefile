TAG     := trex2
VERSION := 2018_05_31
GITDIR  := $(abspath ../..)
ifeq ($(OS),Windows_NT)
	VOLUME   := /$(subst :,,$(GITDIR))
else
	VOLUME   := $(GITDIR)
endif

all:
	docker build -t $(TAG):$(VERSION) .
	docker run --name trex_agent -it -d --net=host -w $(VOLUME) -v $(VOLUME):$(VOLUME) -t $(TAG):$(VERSION)
	docker exec -it trex_agent sh -c "mkdir -p $(VOLUME)/build && mkdir -p $(VOLUME)/build/dune_build && mkdir -p $(VOLUME)/build/trex_build"
	docker exec -it trex_agent sh -c "if [ ! -d "$(VOLUME)/dune" ]; then git clone https://petternorgren:Kxec6-N_MnG4SeTcAU3G@gitlab.com/aurlab/lsts_toolchain/dune.git $(VOLUME)/dune; fi"
	docker exec -it trex_agent sh -c "cd $(VOLUME)/build/dune_build && cmake ../../dune && make -j2 && make install"
	docker exec -it trex_agent sh -c "cd $(VOLUME)/build/trex_build && cmake -DWITH_CPP11=ON -DWITH_EUROPA=OFF ../../trex2-agent && make -j2 && make install"
	docker exec -it trex_agent sh -c "/bin/bash --login"
shell:
	docker exec -it trex_agent sh -c "/bin/bash --login"
clean:
	docker rm -f trex_agent
