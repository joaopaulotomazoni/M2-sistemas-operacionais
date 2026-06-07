#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
using namespace std;

struct Processo {
    int tempoChegada;
    string nome;
    queue<int> paginas;
    
    int pageFaults = 0;
    int tempoFinal = 0;
    
    int quantumUsado = 0;
};

struct ProcessoBloqueado {
    Processo* processo;
    int tempoBloqueado = 4;
};

void AtualizarLRU(vector<int>& memoriaRam, int pagina, int posicaoRam, int frames);
void ProcessarRamHit(queue<Processo*>& filaProntos, vector<int>& memoriaRam, Processo* processoAtual, int posicaoRam, int frames, int clock, int quantum, int& processosConcluidos);
void ProcessarPageFaults(queue<Processo*>& filaProntos, Processo* processoAtual, int tiquesPenalidadeIO, vector<int>& memoriaRam, vector<ProcessoBloqueado>& filaBloqueados, int frames, int clock);
void ImprimirEstadoRam(const vector<int>& memoriaRam, int frames);

void lerArquivo(ifstream& arquivo, vector<Processo>& filaInicial, int& frames, int& quantum, int& tiquesPenalidadeIO){
    string linha;
    
    getline(arquivo, linha);
    stringstream ssPrimeira(linha);
    ssPrimeira >> frames >> quantum >> tiquesPenalidadeIO;

    while(getline(arquivo, linha)){
        stringstream ssProcessos(linha);
        Processo novoProcesso;

        ssProcessos >> novoProcesso.tempoChegada >> novoProcesso.nome;

        string blocoPaginas;
        ssProcessos >> blocoPaginas; 

        stringstream ssPaginas(blocoPaginas);
        string paginaStr;
        
        while (getline(ssPaginas, paginaStr, ',')) {
            novoProcesso.paginas.push(stoi(paginaStr));
        }

        filaInicial.push_back(novoProcesso);
    }
}

void AdicionarProcessoFilaProntos(vector<Processo>& filaInicial, queue<Processo*>& filaProntos, int clock){
    for(int i =0; i < filaInicial.size(); i++){
        if(filaInicial[i].tempoChegada == clock){ 
            filaProntos.push(&filaInicial[i]);
            cout << "[Tempo " << clock << "] " << filaInicial[i].nome << " chegou e entrou na fila de prontos." << endl;
        }
    }   
}

void VerificarPaginasRam(
    queue<Processo*>& filaProntos, 
    vector<int>& memoriaRam, 
    vector<ProcessoBloqueado>& filaBloqueados, 
    int tiquesPenalidadeIO,
    int clock,
    int frames,
    int quantum,
    int& processosConcluidos
) {
   
    Processo* processoAtual = filaProntos.front();
    
    bool ramHit = false;
    for(int i=0; i<memoriaRam.size(); i++){
        //verifica pagina na ram
        if(
            !processoAtual->paginas.empty() && 
            processoAtual->paginas.front() == memoriaRam[i]
        ){
            ramHit = true;
            int posicaoRam = i;
            ProcessarRamHit(filaProntos, memoriaRam, processoAtual, posicaoRam, frames, clock, quantum, processosConcluidos);
            break;
        }
    }

    if(ramHit == false){
        ProcessarPageFaults(filaProntos, processoAtual, tiquesPenalidadeIO, memoriaRam, filaBloqueados, frames, clock);
        return;
    }    
}

void ProcessarRamHit(
    queue<Processo*>& filaProntos, 
    vector<int>& memoriaRam, 
    Processo* processoAtual, 
    int posicaoRam, 
    int frames, 
    int clock, 
    int quantum,
    int& processosConcluidos
){
    int paginaAtual = processoAtual->paginas.front(); 

    cout << "[Tempo " << clock << "] " << processoAtual->nome << " acessou a pagina " << paginaAtual << " (RAM Hit)." << endl;

    AtualizarLRU(memoriaRam, paginaAtual, posicaoRam, frames);

    ImprimirEstadoRam(memoriaRam, frames);

    processoAtual->paginas.pop();
    processoAtual->quantumUsado++;

    if(processoAtual->paginas.empty()){
        processoAtual->tempoFinal = clock + 1;
        filaProntos.pop();
        processosConcluidos++;
        cout << "[Tempo " << clock << "] " << processoAtual->nome << " concluiu sua execucao. Tempo de Retorno: " << (processoAtual->tempoFinal - processoAtual->tempoChegada) << endl;
    }else if(processoAtual->quantumUsado == quantum){
        cout << "[Tempo " << clock << "] " << processoAtual->nome << " sofreu preempcao (Fim do Quantum)." << endl;
        processoAtual->quantumUsado = 0;
        filaProntos.pop();
        filaProntos.push(processoAtual); 
    }
}

void ProcessarPageFaults(
    queue<Processo*>& filaProntos, 
    Processo* processoAtual, 
    int tiquesPenalidadeIO, 
    vector<int>& memoriaRam, 
    vector<ProcessoBloqueado>& filaBloqueados,
    int frames,
    int clock
){
    int paginaPedida = processoAtual->paginas.front();
    
    cout << "[Tempo " << clock << "] " << processoAtual->nome << " sofreu Page Fault na pagina " << paginaPedida << "." << endl;
    
    filaProntos.pop(); 
    
    processoAtual->pageFaults++;
    processoAtual->quantumUsado = 0;
    
    filaBloqueados.push_back({processoAtual, tiquesPenalidadeIO});

    AtualizarLRU(memoriaRam, paginaPedida, -1, frames);

    ImprimirEstadoRam(memoriaRam, frames);
}

void AtualizarLRU(vector<int>& memoriaRam, int pagina, int posicaoRam, int frames){
    if(posicaoRam != -1){ 
        memoriaRam.erase(memoriaRam.begin() + posicaoRam); 
    }else if(memoriaRam.size() == frames){
        memoriaRam.erase(memoriaRam.begin());     
    }

    memoriaRam.push_back(pagina);
}

void ImprimirEstadoRam(const vector<int>& memoriaRam, int frames) {
    cout << "   -> Estado da RAM: ";
    for (int i = 0; i < frames; i++) {
        if (i < memoriaRam.size()) {
            cout << "[ " << memoriaRam[i] << " ] ";
        } else {
            cout << "[ Vazio ] ";
        }
    }
    cout << endl;
}

void AtualizarBloquados(vector<ProcessoBloqueado>& filaBloqueados, queue<Processo*>& filaProntos, int clock){
    int i=0;
    while(i < filaBloqueados.size()){
        filaBloqueados[i].tempoBloqueado--;
        
        if(filaBloqueados[i].tempoBloqueado==0){
            filaProntos.push(filaBloqueados[i].processo);
            cout << "[Tempo " << clock << "] " << filaBloqueados[i].processo->nome << " saiu do I/O e voltou para a fila de prontos." << endl;
            filaBloqueados.erase(filaBloqueados.begin() + i);
        } else {
            i++;
        }
    }
}

int main () {
    ifstream arquivo("arquivo_teste.txt");

    if (!arquivo.is_open()) {
        cout << "Erro ao abrir arquivo";
        return 1;
    }

    vector<Processo> filaInicial;
    int frames, quantum, tiquesPenalidadeIO;

    lerArquivo(arquivo, filaInicial, frames, quantum, tiquesPenalidadeIO);

    int clock = 0;

    int processosConcluidos = 0;

    vector<int> memoriaRam;
    queue<Processo*> filaProntos;
    vector<ProcessoBloqueado> filaBloqueados;

    while(processosConcluidos < filaInicial.size()){
        AdicionarProcessoFilaProntos(filaInicial, filaProntos, clock);

        AtualizarBloquados(filaBloqueados, filaProntos, clock);

        if(!filaProntos.empty()){
            VerificarPaginasRam(filaProntos, memoriaRam, filaBloqueados, tiquesPenalidadeIO, clock, frames, quantum, processosConcluidos);
        } else {
            cout << "[Tempo " << clock << "] CPU ociosa." << endl;
        }

        clock++;
    }
    cout << "\n--- RELATORIO FINAL ---" << endl;
    for (int i = 0; i < filaInicial.size(); i++) {
        int tempoRetorno = filaInicial[i].tempoFinal - filaInicial[i].tempoChegada;
        cout << "Processo: " << filaInicial[i].nome << endl;
        cout << "Tempo de Retorno: " << tempoRetorno << " tiques" << endl;
        cout << "Page Faults: " << filaInicial[i].pageFaults << endl;
        cout << "-----------------------" << endl;
    }
    
    return 0;
}