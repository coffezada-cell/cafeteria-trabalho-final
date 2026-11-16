# Cafeteria — trabalho final

Sistema de gerenciamento de cafeteria em C++17 para a atividade de Laboratório de Programação.

## Compilação

No terminal, dentro desta pasta:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic projeto-final.cpp -o cafeteria
./cafeteria
```

## Funcionalidades

- Registro e atendimento de pedidos em fila FIFO.
- Cardápio, estoque de grãos, descontos promocionais e comprovantes.
- Administração por PIN, alteração de preços e relatório.
- Consulta de pedidos por ID, exportação TXT, exclusão lógica e contagem de registros. As opções 9 a 12 ficam no modo administrador.
- Persistência dos dados em `cafeteria.dat`, criado na pasta de execução.

O programa gera `cafeteria.dat` e, pela opção de exportação, `pedidos.txt`. Esses arquivos representam dados locais de cada execução e não fazem parte do código-fonte.
