# Nome do executável
EXEC = ex1

# Compilador e flags
CXX = g++
CXXFLAGS = -pthread -g -Wall

# Arquivos fonte e objetos
SRC = main.cpp
OBJ = $(SRC:.cpp=.o)

# Regra padrão (default): build
default: build

# Regra para compilar e gerar o executável
build: $(EXEC)

$(EXEC): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Regra para limpar arquivos temporários
clean:
	rm -f $(EXEC) $(OBJ)

# Regra para compilar arquivos .o
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
