
.PHONY : clean all auth network postgres mysql oracle nodb resources api microservices re

nodb : api auth network resources microservices re

re :
	@$(MAKE) -C re

api :
	@$(MAKE) -C api

auth :
	@$(MAKE) -C auth

network :
	@$(MAKE) -C network

postgres :
	@$(MAKE) -C database postgres

mysql :
	@$(MAKE) -C database mysql

oracle :
	@$(MAKE) -C database oracle

resources :
	@$(MAKE) -C resources

microservices :
	@$(MAKE) -C microservices

clean :
	@echo "Cleaning API plugins..."
	@$(MAKE) clean -C api > /dev/null 2>&1
	@echo "Cleaning Authentication plugins..."
	@$(MAKE) clean -C auth > /dev/null 2>&1
	@echo "Cleaning Database plugins..."
	@$(MAKE) clean -C database > /dev/null 2>&1
	@echo "Cleaning Network plugins..."
	@$(MAKE) clean -C network > /dev/null 2>&1
	@echo "Cleaning Resource plugins..."
	@$(MAKE) clean -C resources > /dev/null 2>&1
	@echo "Cleaning Microservice plugins..."
	@$(MAKE) clean -C microservices > /dev/null 2>&1
