
.PHONY : clean all auth network database nodb resources

all : auth database network resources

nodb : auth network resources

auth :
	@$(MAKE) -C auth

database :
	@$(MAKE) -C database

network :
	@$(MAKE) -C network

resources :
	@$(MAKE) -C resources

clean :
	@echo "Cleaning Authentication plugins..."
	@$(MAKE) clean -C auth > /dev/null 2>&1
	@echo "Cleaning Database plugins..."
	@$(MAKE) clean -C database > /dev/null 2>&1
	@echo "Cleaning Network plugins..."
	@$(MAKE) clean -C network > /dev/null 2>&1
	@echo "Cleaning Resource plugins..."
	@$(MAKE) clean -C resources > /dev/null 2>&1
