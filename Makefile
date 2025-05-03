# Nome do executável
EXEC = ex1

# Compilador e flags
CXX = g++
CXXFLAGS = -pthread -g -Wall

# Arquivos fonte e objetos
SRC = main.cpp passa_tempo.c
OBJ = main.o passa_tempo.o

# Regra padrão (default): build
default: build

# Regra para compilar e gerar o executável
build: $(EXEC)

$(EXEC): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Regras para compilar arquivos .o
main.o: main.cpp passa_tempo.h
	$(CXX) $(CXXFLAGS) -c main.cpp -o main.o

passa_tempo.o: passa_tempo.c passa_tempo.h
	$(CXX) $(CXXFLAGS) -c passa_tempo.c -o passa_tempo.o

# Regra para limpar arquivos temporários
clean:
	rm -f $(EXEC) $(OBJ)
