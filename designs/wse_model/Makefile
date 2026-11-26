PYTHON ?= python3

.PHONY: check unit contract integration lint

check: lint unit contract

lint:
	$(PYTHON) -m ruff check .

unit:
	$(PYTHON) -m pytest tests/unit tests/golden

contract:
	$(PYTHON) -m pytest tests/contracts

integration:
	$(PYTHON) -m pytest tests/integration
