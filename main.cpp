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

void lerArquivo(ifstream& arquivo, vector<Processo>& filaInicial, int& frames, int& quantum, int& tiquesPenalidadeIO){
    string linha;
    getline(arquivo, linha);
    stringstream ssPrimeira(linha);

    ssPrimeira >> frames >> quantum >> tiquesPenalidadeIO;

    while(getline(arquivo, linha)){
        stringstream ssProcessos(linha);
        Processo novoProcesso;

        ssProcessos >> novoProcesso.tempoChegada >> novoProcesso.nome;

        int pagina;
        while(ssProcessos >> pagina) {
            novoProcesso.paginas.push(pagina);
        }

        filaInicial.push_back(novoProcesso);
    }
}

void AdicionarProcessoFilaProntos(vector<Processo>& filaInicial, queue<Processo*>& filaProntos, int clock){
    for(int i =0; i < filaInicial.size(); i++){
        if(filaInicial[i].tempoChegada == clock){
            filaProntos.push(&filaInicial[i]);
        }
    }   
}

void VerificarPaginasRam(
    queue<Processo*>& filaProntos, 
    vector<int>& memoriaRam, 
    vector<ProcessoBloqueado>& filaBloqueados, 
    int tiquesPenalidadeIO,
    int clock
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
            RamHit();
            break;
        }
    }

    if(ramHit == false){
        PageFault(filaProntos, processoAtual, tiquesPenalidadeIO, memoriaRam, filaBloqueados);
        return;
    }    
}
void RamHit(){

}

void PageFault(
    queue<Processo*>& filaProntos, 
    Processo* processoAtual, 
    int tiquesPenalidadeIO, 
    vector<int>& memoriaRam, 
    vector<ProcessoBloqueado>& filaBloqueados
){
    filaProntos.pop(); 
    processoAtual->pageFaults++;
    
    filaBloqueados.push_back({processoAtual, tiquesPenalidadeIO});

    for(int i=0; i<memoriaRam.size(); i++){
        if(memoriaRam[i] == -1){
            memoriaRam[i] = processoAtual->paginas.front();
            break;
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

    vector<int> memoriaRam(frames, -1);
    queue<Processo*> filaProntos;
    vector<ProcessoBloqueado> filaBloqueados;

    while(true){

        AdicionarProcessoFilaProntos(filaInicial, filaProntos, clock);

        if(!filaProntos.empty()){
            VerificarPaginasRam(filaProntos, memoriaRam, filaBloqueados, tiquesPenalidadeIO, clock);
        }

        clock++;
    }
}