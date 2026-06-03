# Multisensor docker — common operations.
# Run `make help` for the list.

SHELL := /bin/bash
COMPOSE := docker compose
.DEFAULT_GOAL := help

.PHONY: help setup up down shell build rebuild logs clean nuke status

help: ## Show this help
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "  \033[36m%-12s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST)

setup: ## First-time setup (generates .env, imports src, builds image, builds workspace)
	@./scripts/setup.sh

up: ## Start the container in the background
	@bash -c 'source ./scripts/_x11_setup.sh && $(COMPOSE) up -d'

down: ## Stop and remove the container
	@$(COMPOSE) down

shell: ## Open a shell inside the running container
	@./scripts/shell.sh

build: ## Rebuild the Docker image (does NOT touch the colcon workspace)
	@$(COMPOSE) build

rebuild: ## Rebuild the colcon workspace inside the container
	@$(COMPOSE) exec multisensor /usr/local/bin/entrypoint.sh colcon build --symlink-install

logs: ## Tail container logs
	@$(COMPOSE) logs -f

status: ## Show container status
	@$(COMPOSE) ps

clean: ## Remove the colcon build/install/log caches (keeps the image)
	@$(COMPOSE) exec multisensor bash -lc "rm -rf build install log" || true
	@echo "Workspace caches cleared. Run 'make rebuild' to recompile."