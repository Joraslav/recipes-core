#!/bin/bash

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

LIST_ONLY=false

if [ $# -gt 1 ]; then
	echo -e "${RED}✗ Неверное количество аргументов. Использование: $0 [--list]${NC}"
	exit 1
fi

if [ $# -eq 1 ]; then
	if [ "$1" = "--list" ]; then
		LIST_ONLY=true
	else
		echo -e "${RED}✗ Неизвестный аргумент: $1. Использование: $0 [--list]${NC}"
		exit 1
	fi
fi

if ! command -v clang-format &> /dev/null; then
	echo -e "${RED}✗ clang-format не найден. Установите его и повторите попытку.${NC}"
	exit 1
fi

echo -e "${YELLOW}Поиск C++ файлов для форматирования...${NC}"

mapfile -t cpp_files < <(
	if [ -d "./src" ]; then
		find ./src -type f \
			\( -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" -o -name "*.c" \
			   -o -name "*.hpp" -o -name "*.hh" -o -name "*.hxx" -o -name "*.h" \)
	fi

	if [ -d "./tests" ]; then
		find ./tests -type f \
			\( -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" -o -name "*.c" \
			   -o -name "*.hpp" -o -name "*.hh" -o -name "*.hxx" -o -name "*.h" \)
	fi

	if [ -d "./bench" ]; then
		find ./bench -type f \
			\( -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" -o -name "*.c" \
			   -o -name "*.hpp" -o -name "*.hh" -o -name "*.hxx" -o -name "*.h" \)
	fi

	if [ -f "./main.cpp" ]; then
		echo "./main.cpp"
	fi
)

if [ ${#cpp_files[@]} -eq 0 ]; then
	echo -e "${YELLOW}C++ файлы не найдены.${NC}"
	exit 0
fi

if [ "$LIST_ONLY" = true ]; then
	echo -e "${YELLOW}Найдено ${#cpp_files[@]} файлов:${NC}"
	printf '%s\n' "${cpp_files[@]}"
	exit 0
fi

echo -e "${YELLOW}Найдено ${#cpp_files[@]} файлов. Запускаю clang-format...${NC}"

clang-format -i "${cpp_files[@]}"

echo -e "${GREEN}✓ Форматирование завершено успешно!${NC}"
