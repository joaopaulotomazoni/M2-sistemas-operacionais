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

void HandleLru(vector<int>& memoriaRam, int pagina, int posicaoRam, int frames);
void RamHit(queue<Processo*>& filaProntos, vector<int>& memoriaRam, Processo* processoAtual, int posicaoRam, int frames, int clock, int quantum, int& processosConcluidos);
void PageFault(queue<Processo*>& filaProntos, Processo* processoAtual, int tiquesPenalidadeIO, vector<int>& memoriaRam, vector<ProcessoBloqueado>& filaBloqueados, int frames);

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
        if(filaInicial[i].tempoChegada == clock){ //fazer para adicionar a fila de prontos aqueles processos que estavam na fila de bloquados com o tempoBloquado = 0
            filaProntos.push(&filaInicial[i]);
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
    //Pega primeiro processo da fila de prontos 
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
            RamHit(filaProntos, memoriaRam, processoAtual, posicaoRam, frames, clock, quantum, processosConcluidos);
            break;
        }
    }

    if(ramHit == false){
        PageFault(filaProntos, processoAtual, tiquesPenalidadeIO, memoriaRam, filaBloqueados, frames);
        return;
    }    
}

void RamHit(
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
    HandleLru(memoriaRam, paginaAtual, posicaoRam, frames);

    processoAtual->paginas.pop();
    processoAtual->quantumUsado++;

    if(processoAtual->paginas.empty()){
        processoAtual->tempoFinal = clock;
        filaProntos.pop();
        processosConcluidos++;
    }else if(processoAtual->quantumUsado == quantum){
        processoAtual->quantumUsado=0;
        filaProntos.pop();
        filaProntos.push(processoAtual); //passa o processo para o fim da fila
    }
}

void PageFault(
    queue<Processo*>& filaProntos, 
    Processo* processoAtual, 
    int tiquesPenalidadeIO, 
    vector<int>& memoriaRam, 
    vector<ProcessoBloqueado>& filaBloqueados,
    int frames
){
    filaProntos.pop(); 

    processoAtual->pageFaults++;
    filaBloqueados.push_back({processoAtual, tiquesPenalidadeIO});

    int paginaPedida = processoAtual->paginas.front();
    
    HandleLru(memoriaRam, paginaPedida, -1, frames);
}

void HandleLru(vector<int>& memoriaRam, int pagina, int posicaoRam, int frames){
    if(posicaoRam != -1){ //remove a pagina da posicao atual, idependente de a Ram estar cheia
        memoriaRam.erase(memoriaRam.begin() + posicaoRam); //apaga o primeiro
    }else if(memoriaRam.size() == frames){// pega o tamanho
        memoriaRam.erase(memoriaRam.begin()); //apaga o primeiro    
    }

    memoriaRam.push_back(pagina);
}

void AtualizarBloquados(vector<ProcessoBloqueado>& filaBloqueados, queue<Processo*>& filaProntos){
    int i=0;
    while(i < filaBloqueados.size()){
        filaBloqueados[i].tempoBloqueado--;
        
        if(filaBloqueados[i].tempoBloqueado==0){
            filaProntos.push(filaBloqueados[i].processo);
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

    vector<int> memoriaRam(frames);
    queue<Processo*> filaProntos;
    vector<ProcessoBloqueado> filaBloqueados;

    while(processosConcluidos < filaInicial.size()){
        AdicionarProcessoFilaProntos(filaInicial, filaProntos, clock);

        AtualizarBloquados(filaBloqueados, filaProntos);

        if(!filaProntos.empty()){
            VerificarPaginasRam(filaProntos, memoriaRam, filaBloqueados, tiquesPenalidadeIO, clock, frames, quantum, processosConcluidos);
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