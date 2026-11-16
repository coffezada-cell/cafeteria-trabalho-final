#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <cstdio>

using namespace std;

const int MAX_FILA = 20;
const int MAX_REGISTROS = 1000;
const int QTD_ITENS = 5;
const int GRAMAS_POR_DOSE = 18;
const int PIN_ADMIN = 1234;
const char *ARQUIVO_DADOS = "cafeteria.dat";
const char *ARQUIVO_TXT = "pedidos.txt";

struct Pedido {
    int numero;
    int idItem;
    int quantidade;
    float total;
    int ativo;      // 1 = cadastrado; 0 = excluido
    int atendido;   // 1 = ja saiu da fila FIFO
};

struct EstadoArquivo {
    char assinatura[8];
    unsigned int versao;
    int estoqueGramas;
    float faturamento;
    int totalPedidos;
    float precos[QTD_ITENS];
    int codigoPromocional;
    int dataPromocional;
    unsigned int qtdRegistros;
};

// Cardapio (indice 0 = item 1)
const string nomesItens[QTD_ITENS] = {"Espresso", "Espresso duplo", "Cappuccino", "Coado V60", "Pao de queijo"};
float precosItens[QTD_ITENS] = {6.00f, 9.00f, 12.00f, 10.00f, 5.50f};
const int dosesItens[QTD_ITENS] = {1, 2, 1, 1, 0};
Pedido registros[MAX_REGISTROS];
int qtdRegistros = 0;
int codigoPromocional = 0;
int dataPromocional = 0;

// Fila circular (FIFO) de pedidos
Pedido fila[MAX_FILA];
int inicioFila = 0;
int fimFila = 0;
int qtdFila = 0;

// ---------- Leitura segura ----------

int lerInteiro() {
    string linha;
    int valor;

    while (true) {
        if (!getline(cin, linha)) {
            return 0;
        }
        istringstream leitor(linha);
        if (leitor >> valor) {
            return valor;
        }
        cout << "Entrada invalida. Digite um numero: ";
    }
}

float lerFloat() {
    string linha;
    float valor;

    while (true) {
        if (!getline(cin, linha)) {
            return 0;
        }
        replace(linha.begin(), linha.end(), ',', '.');
        istringstream leitor(linha);
        if (leitor >> valor) {
            return valor;
        }
        cout << "Entrada invalida. Digite um valor: ";
    }
}

// ---------- Funcoes do enunciado ----------

// Sem parametro, com retorno: codigo fixo do dia (sorteado uma unica vez)
int obterCodigoPromocional() {
    if (codigoPromocional == 0) {
        codigoPromocional = (rand() % 3) + 1;
    }
    return codigoPromocional;
}

int obterDescontoPercentual(int codigo) {
    if (codigo == 1) {
        return 5;
    } else if (codigo == 2) {
        return 10;
    } else if (codigo == 3) {
        return 15;
    } else {
        return 0;
    }
}

// Com parametro, sem retorno
void imprimirComprovantePedido(int idItem, float precoTotal) {
    cout << "\n====== COMPROVANTE ======\n";
    cout << "Item : " << nomesItens[idItem - 1] << "\n";
    cout << "Total: R$ " << precoTotal << "\n";
    cout << "=========================\n";
}

// Valor: quantidadeDoses | Referencia: estoqueGramas
void baixarEstoqueGrains(int quantidadeDoses, int &estoqueGramas) {
    int consumo = quantidadeDoses * GRAMAS_POR_DOSE;

    if (consumo <= estoqueGramas) {
        estoqueGramas = estoqueGramas - consumo;
    } else {
        cout << "Estoque insuficiente para baixar.\n";
    }
}

// ---------- Fila FIFO ----------

bool filaCheia() {
    if (qtdFila == MAX_FILA) {
        return true;
    } else {
        return false;
    }
}

bool filaVazia() {
    if (qtdFila == 0) {
        return true;
    } else {
        return false;
    }
}

void enfileirar(Pedido p) {
    fila[fimFila] = p;
    fimFila = (fimFila + 1) % MAX_FILA;
    qtdFila++;
}

Pedido desenfileirar() {
    Pedido p = fila[inicioFila];
    inicioFila = (inicioFila + 1) % MAX_FILA;
    qtdFila--;
    return p;
}

void reconstruirFila() {
    inicioFila = fimFila = qtdFila = 0;
    for (int i = 0; i < qtdRegistros; i++) {
        const Pedido &p = registros[i];
        if (p.ativo && !p.atendido) {
            enfileirar(p);
        }
    }
}

void exibirFila() {
    int posicao;
    Pedido p;

    if (filaVazia()) {
        cout << "\nNenhum pedido na fila.\n";
        return;
    }

    cout << "\n--- Fila de pedidos (proximo no topo) ---\n";
    for (int i = 0; i < qtdFila; i++) {
        posicao = (inicioFila + i) % MAX_FILA;
        p = fila[posicao];
        cout << i + 1 << ") Pedido #" << p.numero << " - " << p.quantidade << "x "
             << nomesItens[p.idItem - 1] << " - R$ " << p.total << "\n";
    }
}

// ---------- Arquivo binario e registros ----------

int dataDeHoje() {
    time_t agora = time(nullptr);
    tm data = *localtime(&agora);
    return (data.tm_year + 1900) * 10000 + (data.tm_mon + 1) * 100 + data.tm_mday;
}

bool salvarEstado(int estoqueGramas, float faturamento, int totalPedidos) {
    EstadoArquivo cabecalho = {};
    string marca = "CAFETER1";
    for (int i = 0; i < 8; i++) cabecalho.assinatura[i] = marca[i];
    cabecalho.versao = 1;
    cabecalho.estoqueGramas = estoqueGramas;
    cabecalho.faturamento = faturamento;
    cabecalho.totalPedidos = totalPedidos;
    for (int i = 0; i < QTD_ITENS; i++) cabecalho.precos[i] = precosItens[i];
    cabecalho.codigoPromocional = codigoPromocional;
    cabecalho.dataPromocional = dataPromocional;
    cabecalho.qtdRegistros = qtdRegistros;

    ofstream arquivo("cafeteria.tmp", ios::binary);
    if (!arquivo) return false;
    arquivo.write(reinterpret_cast<const char *>(&cabecalho), sizeof(cabecalho));
    for (int i = 0; i < qtdRegistros; i++) {
        arquivo.write(reinterpret_cast<const char *>(&registros[i]), sizeof(Pedido));
    }
    arquivo.close();
    if (!arquivo) {
        remove("cafeteria.tmp");
        return false;
    }
    if (rename("cafeteria.tmp", ARQUIVO_DADOS) != 0) {
        remove("cafeteria.tmp");
        return false;
    }
    return true;
}

bool carregarEstado(int &estoqueGramas, float &faturamento, int &totalPedidos) {
    ifstream arquivo(ARQUIVO_DADOS, ios::binary);
    if (!arquivo) return true; // Primeira execucao: ainda nao existe arquivo.
    EstadoArquivo cabecalho = {};
    if (!arquivo.read(reinterpret_cast<char *>(&cabecalho), sizeof(cabecalho))) return false;
    string marca = "CAFETER1";
    for (int i = 0; i < 8; i++) {
        if (cabecalho.assinatura[i] != marca[i]) return false;
    }
    if (cabecalho.versao != 1 ||
        cabecalho.qtdRegistros > MAX_REGISTROS ||
        cabecalho.totalPedidos != static_cast<int>(cabecalho.qtdRegistros) ||
        cabecalho.estoqueGramas < 0 || cabecalho.estoqueGramas > 100000000 ||
        !(cabecalho.faturamento >= 0 && cabecalho.faturamento <= 1000000000000.0f) ||
        cabecalho.codigoPromocional < 1 ||
        cabecalho.codigoPromocional > 3) return false;

    for (int i = 0; i < QTD_ITENS; i++) {
        if (!(cabecalho.precos[i] > 0 && cabecalho.precos[i] <= 1000000)) return false;
    }
    Pedido lidos[MAX_REGISTROS];
    int pendentes = 0;
    for (int i = 0; i < cabecalho.totalPedidos; i++) {
        if (!arquivo.read(reinterpret_cast<char *>(&lidos[i]), sizeof(Pedido))) return false;
        const Pedido &p = lidos[i];
        if (p.numero != i + 1 || p.idItem < 1 || p.idItem > QTD_ITENS ||
            p.quantidade < 1 || p.quantidade > 1000 ||
            !(p.total >= 0 && p.total <= 1000000000.0f) ||
            (p.ativo != 0 && p.ativo != 1) || (p.atendido != 0 && p.atendido != 1)) return false;
        if (p.ativo && !p.atendido) ++pendentes;
    }
    if (pendentes > MAX_FILA || arquivo.peek() != EOF) return false;

    qtdRegistros = cabecalho.totalPedidos;
    for (int i = 0; i < qtdRegistros; i++) registros[i] = lidos[i];
    estoqueGramas = cabecalho.estoqueGramas;
    faturamento = cabecalho.faturamento;
    totalPedidos = cabecalho.totalPedidos;
    for (int i = 0; i < QTD_ITENS; i++) precosItens[i] = cabecalho.precos[i];
    codigoPromocional = cabecalho.codigoPromocional;
    dataPromocional = cabecalho.dataPromocional;
    reconstruirFila();
    return true;
}

void consultarPedido() {
    cout << "Codigo/ID do pedido: ";
    int id = lerInteiro();
    if (id < 1 || id > qtdRegistros) {
        cout << "Pedido nao encontrado.\n";
        return;
    }
    const Pedido &p = registros[id - 1];
    cout << "\nPedido #" << p.numero << "\n"
         << "Item       : " << nomesItens[p.idItem - 1] << "\n"
         << "Quantidade : " << p.quantidade << "\n"
         << "Total      : R$ " << p.total << "\n"
         << "Status     : " << (p.ativo ? "ativo" : "excluido") << "\n"
         << "Atendimento: " << (p.atendido ? "atendido" : "pendente") << "\n";
}

void exportarPedidos() {
    ofstream arquivo(ARQUIVO_TXT, ios::trunc);
    if (!arquivo) {
        cout << "Nao foi possivel criar o arquivo TXT.\n";
        return;
    }
    arquivo << fixed << setprecision(2);
    arquivo << "REGISTROS DE PEDIDOS DA CAFETERIA\n";
    arquivo << "ID | Item | Quantidade | Total (R$) | Status | Atendimento\n";
    for (int i = 0; i < qtdRegistros; i++) {
        const Pedido &p = registros[i];
        arquivo << p.numero << " | " << nomesItens[p.idItem - 1] << " | "
                << p.quantidade << " | " << p.total << " | "
                << (p.ativo ? "ativo" : "excluido") << " | "
                << (p.atendido ? "atendido" : "pendente") << "\n";
    }
    arquivo.close();
    cout << (arquivo ? "Registros exportados para pedidos.txt.\n"
                     : "Falha ao gravar o arquivo TXT.\n");
}

void contarPedidos() {
    int ativos = 0;
    int atendidos = 0;
    for (int i = 0; i < qtdRegistros; i++) {
        const Pedido &p = registros[i];
        if (p.ativo) {
            ++ativos;
            if (p.atendido) ++atendidos;
        }
    }
    cout << "\nRegistros armazenados: " << qtdRegistros
         << "\nRegistros ativos     : " << ativos
         << "\nRegistros excluidos  : " << qtdRegistros - ativos
         << "\nAtendidos ativos     : " << atendidos
         << "\nPendentes na fila    : " << qtdFila << "\n";
}

void excluirPedido(int &estoqueGramas, float &faturamento, int totalPedidos) {
    cout << "Codigo/ID do pedido a excluir: ";
    int id = lerInteiro();
    if (id < 1 || id > qtdRegistros || !registros[id - 1].ativo) {
        cout << "Pedido ativo nao encontrado.\n";
        return;
    }
    Pedido &p = registros[id - 1];
    int devolucao = p.atendido ? 0 :
        dosesItens[p.idItem - 1] * p.quantidade * GRAMAS_POR_DOSE;
    if (estoqueGramas + devolucao > 100000000) {
        cout << "Nao foi possivel devolver o estoque.\n";
        return;
    }
    p.ativo = 0;
    float novoFaturamento = 0.0f;
    for (int i = 0; i < qtdRegistros; i++) {
        const Pedido &registro = registros[i];
        if (registro.ativo) novoFaturamento += registro.total;
    }
    if (!salvarEstado(estoqueGramas + devolucao, novoFaturamento, totalPedidos)) {
        p.ativo = 1;
        cout << "Falha ao salvar. Exclusao cancelada.\n";
        return;
    }
    estoqueGramas += devolucao;
    faturamento = novoFaturamento;
    reconstruirFila();
    cout << "Pedido #" << id << " excluido.\n";
}

// ---------- Operacoes do caixa ----------

void exibirCardapio() {
    cout << "\n--- Cardapio ---\n";
    for (int i = 0; i < QTD_ITENS; i++) {
        cout << i + 1 << ") " << left << setw(16) << nomesItens[i] << " R$ " << precosItens[i] << "\n";
    }
}

bool estoqueSuficiente(int quantidadeDoses, int estoqueGramas) {
    if (quantidadeDoses * GRAMAS_POR_DOSE <= estoqueGramas) {
        return true;
    } else {
        return false;
    }
}

void novoPedido(int &estoqueGramas, float &faturamento, int &totalPedidos) {
    int idItem;
    int quantidade;
    int doses;
    int codigo;
    int desconto;
    float total;
    Pedido p;

    if (qtdRegistros == MAX_REGISTROS) {
        cout << "\nLimite de registros atingido.\n";
        return;
    }
    if (filaCheia()) {
        cout << "\nFila cheia! Atenda algum pedido antes de registrar outro.\n";
        return;
    }

    exibirCardapio();
    cout << "Item (1-" << QTD_ITENS << "): ";
    idItem = lerInteiro();
    if (idItem < 1 || idItem > QTD_ITENS) {
        cout << "Item invalido.\n";
        return;
    }

    cout << "Quantidade: ";
    quantidade = lerInteiro();
    if (quantidade < 1 || quantidade > 1000) {
        cout << "Quantidade invalida.\n";
        return;
    }

    doses = dosesItens[idItem - 1] * quantidade;
    if (!estoqueSuficiente(doses, estoqueGramas)) {
        cout << "Estoque de graos insuficiente para este pedido.\n";
        return;
    }

    codigo = obterCodigoPromocional();
    desconto = obterDescontoPercentual(codigo);
    total = precosItens[idItem - 1] * static_cast<float>(quantidade);
    total = total - (total * static_cast<float>(desconto) / 100.0f);
    if (!(total >= 0 && total <= 1000000000.0f) ||
        !(faturamento + total <= 1000000000000.0f)) {
        cout << "Pedido excede os limites do sistema.\n";
        return;
    }

    p.numero = totalPedidos + 1;
    p.idItem = idItem;
    p.quantidade = quantidade;
    p.total = total;
    p.ativo = 1;
    p.atendido = 0;
    registros[qtdRegistros] = p;
    qtdRegistros++;
    int novoEstoque = estoqueGramas - doses * GRAMAS_POR_DOSE;
    if (!salvarEstado(novoEstoque, faturamento + total, p.numero)) {
        qtdRegistros--;
        cout << "Falha ao salvar. Pedido nao registrado.\n";
        return;
    }
    baixarEstoqueGrains(doses, estoqueGramas);
    totalPedidos++;
    faturamento += total;
    enfileirar(p);

    cout << "\nPedido #" << p.numero << " registrado. Cupom do dia: codigo " << codigo
         << " (" << desconto << "% de desconto)\n";
    imprimirComprovantePedido(idItem, total);

    if (estoqueGramas < 500) {
        cout << "Atencao: estoque de graos baixo (" << estoqueGramas << " g).\n";
    }
}

void atenderPedido(int estoqueGramas, float faturamento, int totalPedidos) {
    Pedido p;

    if (filaVazia()) {
        cout << "\nNao ha pedidos na fila.\n";
        return;
    }

    p = fila[inicioFila];
    registros[p.numero - 1].atendido = 1;
    if (!salvarEstado(estoqueGramas, faturamento, totalPedidos)) {
        registros[p.numero - 1].atendido = 0;
        cout << "Falha ao salvar. Pedido permanece na fila.\n";
        return;
    }
    p = desenfileirar();
    cout << "\nAtendendo pedido #" << p.numero << ": " << p.quantidade << "x "
         << nomesItens[p.idItem - 1] << "\n";
}

// ---------- Administracao ----------

bool alternarModoAdmin(bool modoAtual) {
    int pin;

    if (modoAtual) {
        cout << "\nSaindo do modo administracao.\n";
        return false;
    }

    cout << "PIN de administrador: ";
    pin = lerInteiro();
    if (pin == PIN_ADMIN) {
        cout << "\nModo administracao ativado.\n";
        return true;
    } else {
        cout << "\nPIN incorreto.\n";
        return false;
    }
}

void reporEstoque(int &estoqueGramas, float faturamento, int totalPedidos) {
    int gramas;

    cout << "Estoque atual: " << estoqueGramas << " g\n";
    cout << "Gramas a repor: ";
    gramas = lerInteiro();
    if (gramas < 1 || gramas > 100000000 - estoqueGramas) {
        cout << "Valor invalido.\n";
        return;
    }
    if (!salvarEstado(estoqueGramas + gramas, faturamento, totalPedidos)) {
        cout << "Falha ao salvar. Estoque nao alterado.\n";
        return;
    }
    estoqueGramas = estoqueGramas + gramas;
    cout << "Novo estoque: " << estoqueGramas << " g\n";
}

void alterarPreco(int estoqueGramas, float faturamento, int totalPedidos) {
    int idItem;
    float novoPreco;

    exibirCardapio();
    cout << "Item a alterar (1-" << QTD_ITENS << "): ";
    idItem = lerInteiro();
    if (idItem < 1 || idItem > QTD_ITENS) {
        cout << "Item invalido.\n";
        return;
    }

    cout << "Novo preco: ";
    novoPreco = lerFloat();
    if (!(novoPreco > 0 && novoPreco <= 1000000)) {
        cout << "Preco invalido.\n";
        return;
    }
    float precoAnterior = precosItens[idItem - 1];
    precosItens[idItem - 1] = novoPreco;
    if (!salvarEstado(estoqueGramas, faturamento, totalPedidos)) {
        precosItens[idItem - 1] = precoAnterior;
        cout << "Falha ao salvar. Preco nao alterado.\n";
        return;
    }
    cout << nomesItens[idItem - 1] << " agora custa R$ " << novoPreco << "\n";
}

void exibirRelatorio(int estoqueGramas, float faturamento, int totalPedidos) {
    cout << "\n====== RELATORIO ======\n";
    int ativos = 0, atendidos = 0;
    for (int i = 0; i < qtdRegistros; i++) {
        const Pedido &p = registros[i];
        if (p.ativo) {
            ++ativos;
            if (p.atendido) ++atendidos;
        }
    }
    cout << "Pedidos registrados : " << totalPedidos << "\n";
    cout << "Pedidos ativos      : " << ativos << "\n";
    cout << "Pedidos excluidos   : " << totalPedidos - ativos << "\n";
    cout << "Pedidos atendidos   : " << atendidos << "\n";
    cout << "Pedidos na fila     : " << qtdFila << "\n";
    cout << "Faturamento         : R$ " << faturamento << "\n";
    cout << "Estoque de graos    : " << estoqueGramas << " g (~" << estoqueGramas / GRAMAS_POR_DOSE << " doses)\n";
    cout << "Codigo promo do dia : " << obterCodigoPromocional() << "\n";
    cout << "=======================\n";
}

// ---------- Menu ----------

void exibirMenu(bool modoAdmin) {
    if (modoAdmin) {
        cout << "\n===== CAFETERIA [ADMIN] =====\n";
    } else {
        cout << "\n===== CAFETERIA [CAIXA] =====\n";
    }
    cout << "1 - Novo pedido\n";
    cout << "2 - Atender proximo da fila\n";
    cout << "3 - Ver fila\n";
    cout << "4 - Ver cardapio\n";
    if (modoAdmin) {
        cout << "5 - Sair do modo administracao\n";
        cout << "6 - Repor estoque de graos\n";
        cout << "7 - Alterar preco\n";
        cout << "8 - Relatorio\n";
        cout << "9 - Consultar pedido por ID\n";
        cout << "10 - Exportar pedidos para TXT\n";
        cout << "11 - Excluir pedido\n";
        cout << "12 - Contabilizar registros\n";
    } else {
        cout << "5 - Entrar no modo administracao\n";
    }
    cout << "0 - Encerrar\n";
    cout << "Opcao: ";
}

int main() {
    int opcao;
    bool modoAdmin = false;
    int estoqueGramas = 5000;
    float faturamento = 0.0f;
    int totalPedidos = 0;

    srand(static_cast<unsigned>(time(NULL)));
    cout << fixed << setprecision(2);
    if (!carregarEstado(estoqueGramas, faturamento, totalPedidos)) {
        cerr << "Erro: arquivo cafeteria.dat invalido ou inacessivel. Dados preservados.\n";
        return 1;
    }
    if (dataPromocional != dataDeHoje()) {
        dataPromocional = dataDeHoje();
        codigoPromocional = 0;
        obterCodigoPromocional();
        if (!salvarEstado(estoqueGramas, faturamento, totalPedidos)) {
            cerr << "Erro: nao foi possivel iniciar o arquivo binario.\n";
            return 1;
        }
    }

    do {
        exibirMenu(modoAdmin);
        opcao = lerInteiro();

        switch (opcao) {
            case 1:
                novoPedido(estoqueGramas, faturamento, totalPedidos);
                break;
            case 2:
                atenderPedido(estoqueGramas, faturamento, totalPedidos);
                break;
            case 3:
                exibirFila();
                break;
            case 4:
                exibirCardapio();
                break;
            case 5:
                modoAdmin = alternarModoAdmin(modoAdmin);
                break;
            case 6:
                if (modoAdmin) {
                    reporEstoque(estoqueGramas, faturamento, totalPedidos);
                } else {
                    cout << "\nAcesso restrito ao modo administracao.\n";
                }
                break;
            case 7:
                if (modoAdmin) {
                    alterarPreco(estoqueGramas, faturamento, totalPedidos);
                } else {
                    cout << "\nAcesso restrito ao modo administracao.\n";
                }
                break;
            case 8:
                if (modoAdmin) {
                    exibirRelatorio(estoqueGramas, faturamento, totalPedidos);
                } else {
                    cout << "\nAcesso restrito ao modo administracao.\n";
                }
                break;
            case 9:
                if (modoAdmin) {
                    consultarPedido();
                } else {
                    cout << "\nAcesso restrito ao modo administracao.\n";
                }
                break;
            case 10:
                if (modoAdmin) {
                    exportarPedidos();
                } else {
                    cout << "\nAcesso restrito ao modo administracao.\n";
                }
                break;
            case 11:
                if (modoAdmin) {
                    excluirPedido(estoqueGramas, faturamento, totalPedidos);
                } else {
                    cout << "\nAcesso restrito ao modo administracao.\n";
                }
                break;
            case 12:
                if (modoAdmin) {
                    contarPedidos();
                } else {
                    cout << "\nAcesso restrito ao modo administracao.\n";
                }
                break;
            case 0:
                cout << "\nEncerrando. Ate logo!\n";
                break;
            default:
                cout << "\nOpcao invalida.\n";
        }
    } while (opcao != 0);

    return 0;
}
