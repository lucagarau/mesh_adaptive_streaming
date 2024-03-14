#include <cinolib/meshes/drawable_trimesh.h>
#include <cinolib/gl/surface_mesh_controls.h>
#include <cinolib/gl/glcanvas.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>

//#define INPUT_PATH R"(C:\Users\luca-\OneDrive\Data\progetti\Borsa\lib\common-3d-test-models\data)"
//#define INPUT_PATH R"(C:\Users\luca-\OneDrive\Data\progetti\Borsa\cinomuletto\meshes\test)"
//#define OUTPUT_PATH R"(C:\Users\luca-\OneDrive\Data\progetti\Borsa\test_compressione\meshes\compressed)"
//#define DECO_PATH R"(C:\Users\luca-\OneDrive\Data\progetti\Borsa\test_compressione\meshes\decompressed)"
//#define RESULTS_PATH R"(C:\Users\luca-\OneDrive\Data\progetti\Borsa\test_compressione\results\)"

#define MESH_EXTENSION ".obj"
#define OUTPUT_EXTENSION ".drc"
#define DEBUG_ARG 0

namespace fs = std::filesystem;


struct results{
    //file originale
    std::string model;
    uintmax_t sizeOriginal = 0;
    uint vertOriginal = 0;
    uint edgesOriginal = 0;
    uint polyOriginal = 0;

    //file compresso
    uintmax_t sizeCompressed = 0;
    double compressionTime = 0.0;
    int cl = 10;
    int qp= 11;

    //file decompresso
    uintmax_t sizeDecompressed = 0;
    double decompressionTime = 0.0;
    uint vertDecompressed = 0;
    uint edgesDecompressed = 0;
    uint polyDecompressed = 0;

    //percentuali di perdita di dati
    double sizeLoss = 0.0;
    double vertLoss = 0.0;
    double edgesLoss = 0.0;
    double polyLoss = 0.0;

};

// Definizione del mutex globale per sincronizzare l'accesso al file
std::mutex fileMutex;

//todo aggiornare la scrittura su file
void writeToFile(const std::string& csv_path, const results& ris) {
    // Bloccare il mutex prima di accedere al file
    std::lock_guard<std::mutex> lock(fileMutex);

    //Scrittura dei risultati su file csv
    //se la prima riga è vuota allora scrivo l'header
    if(fs::is_empty(csv_path)){
        std::ofstream file(csv_path, std::ios::app);
        if(file.is_open()){
            file << "model;size_orig;nume_vtx_orig;num_edge_orig;num_poly_orig;compression_time_ms;size_compr;param_cl;param_qp;decompression_time_ms;size_deco;num_vtx_deco;num_edge_deco;num_poly_deco;compression_ratio;vtx_loss;edge_loss;poly_loss" << std::endl;
            file.close();
        }
    }

    // Aprire il file in modalità di append
    std::ofstream file(csv_path, std::ios::app);
    if(file.is_open()) {
        // Scrivere i dati nel file
        file << ris.model << ";" << ris.sizeOriginal << ";" << ris.vertOriginal << ";" << ris.edgesOriginal << ";" << ris.polyOriginal << ";" << ris.compressionTime << ";" << ris.sizeCompressed << ";" << ris.cl << ";" << ris.qp << ";" << ris.decompressionTime << ";" << ris.sizeDecompressed << ";" << ris.vertDecompressed << ";" << ris.edgesDecompressed << ";" << ris.polyDecompressed << ";" << ris.sizeLoss << ";" << ris.vertLoss << ";" << ris.edgesLoss << ";" <<ris.polyLoss << std::endl;
        // Chiudere il file
        file.close();
    } else {
        // Gestire eventuali errori nell'apertura del file
        std::cerr << "Errore nell'apertura del file: " << csv_path << std::endl;
    }
}

int main(int argc, char **argv)
{
    using namespace cinolib;
    using namespace std::chrono;
    using namespace std;
    int cl = -1;
    int qp = -1;
    std::string filename;
    std::string csv_path;


        //Controllo sui parametri in input
        if (argc < 2){
            std::cerr << "Numero di argomenti non valido" << std::endl;
            return -1;
        }
    if(DEBUG_ARG){
        std::cout << "Numero di argomenti: " << argc << std::endl;

        for (int i = 0; i < argc; ++i) {
            std::cout << "Argomento " << i << ": " << argv[i] << std::endl;
        }
    }

    //Controllo sui parametri in input
    if (argc <= 2){
        std::cerr << "Numero di argomenti non valido, usa: \n\t-cl [0-10]\t compression level\n\t-qp [0-30]\t quantization bit\n\t-f\t\tcsv path\n\t-h\t\tmostra l'help" << std::endl;
        return -1;
    }

    if(argc <= 4){
        if (std::string(argv[3]) == "-f"){
            csv_path = std::string(argv[4]);
            cl = 10;
            qp = 11;

            filename = std::string(argv[1]);
        }
        else if (std::string(argv[3]) == "-h"){
            std::cout<< "Esegue test di compressione e decompressione di un modello 3D\n"<< std::endl;
            std::cout<< "\tinput: path del modello 3D\n"<< std::endl;
            std::cout << "usa: \n\t-cl [0-10]\t compression level\n\t-qp [0-30]\t quantization bit\n\t-f\t\tcsv path" << std::endl;
            return 0;
        }else{
            std::cerr << "Errore nei parametri in input, usare -h per l'help" << std::endl;
            return -1;
        }
    }

    if(argc <= 8 ){
        filename = std::string(argv[1]);
        for(int i = 2; i < argc; i++){
            if(std::string(argv[i]) == "-cl"){
                cl = std::stoi(argv[i+1]);
            }
            if(std::string(argv[i]) == "-qp"){
                qp = std::stoi(argv[i+1]);
            }
            if(std::string(argv[i]) == "-f"){
                csv_path = std::string(argv[i+1]);
            }
        }
        if(cl == -1)
            cl = 10;
        if(qp == -1)
            qp = 11;
        if(csv_path.empty()){
            std::cerr << "Numero di argomenti non valido, usa: \n\t-cl [0-10]\t compression level\n\t-qp [0-30]\t quantization bit\n\t-f\t\tcsv path" << std::endl;
            if(DEBUG_ARG){
                std::cout << "cl: " << cl << std::endl;
                std::cout << "qp: " << qp << std::endl;
                std::cout << "csv_path: " << csv_path << std::endl;
            }
            return -1;
        }
    }

    if(argc > 8){
        std::cerr << "Numero di argomenti non valido, usa: \n\t-cl [0-10]\t compression level\n\t-qp [0-30]\t quantization bit\n\t-f\t\tcsv path" << std::endl;
        return -1;
    }


    //dichiarazione variabili di supporto
    std::string command;
    std::vector<results> resultsTable;


    //Creazione delle cartella di output temporanea
    // Ottenere la directory temporanea di sistema
    const char* tempDirPath = std::getenv("TMP");
    if (tempDirPath == nullptr) {
        std::cerr << "Impossibile ottenere la directory temporanea di sistema." << std::endl;
        return 1;
    }

    // Creare una cartella temporanea
    fs::path tempDir = fs::temp_directory_path() / "com_test_tmp";
    if (!fs::exists(tempDir)) {
        if (!fs::create_directory(tempDir)) {
            std::cerr << "Impossibile creare la cartella temporanea." << std::endl;
            return 1;
        }
    }

    string base_name = fs::path(filename).stem().string();

    string fileCompresso = base_name + std::string(OUTPUT_EXTENSION);
    string fileDecompresso = base_name + MESH_EXTENSION;

    string com_path = tempDir.string()+"\\"+fileCompresso;
    string deco_path = tempDir.string()+"\\"+fileDecompresso;

    if(DEBUG_ARG){
        //debug
        cout << "DEBUG" << endl;
        cout << "filename: " << filename << endl;
        cout << "base_name: " << base_name << endl;
        cout << "fileCompresso: " << fileCompresso << endl;
        cout << "fileDecompresso: " << fileDecompresso << endl;
        cout << "com_path: " << com_path << endl;
        cout << "deco_path: " << deco_path << endl;
    }



    results ris;

    std::cout << "Compressione di : " << base_name <<" CL = " << std::to_string(cl) <<" QP " << std::to_string(qp) << std::endl;

    //lettura modello originale
    DrawableTrimesh<> m_originale(filename.c_str());
    ris.model = base_name;
    ris.sizeOriginal = fs::directory_entry(filename.c_str()).file_size();
    ris.vertOriginal = m_originale.num_verts();
    ris.edgesOriginal = m_originale.num_edges();
    ris.polyOriginal = m_originale.num_polys();

    auto start = std::chrono::high_resolution_clock::now();
    //compressione del modello
    command = "draco_encoder -i " + filename + " -o " + com_path + " -cl " + std::to_string(cl) + " -qp " + std::to_string(qp);
    system(command.c_str());
    auto end = std::chrono::high_resolution_clock::now();

    ris.sizeCompressed = fs::directory_entry(com_path).file_size();
    ris.cl = cl;
    ris.qp = qp;

    //calcolo tempo di compressione
    std::chrono::duration<double,std::milli > elapsed = end-start;
    ris.compressionTime = elapsed.count();

    //decompressione del modello
    start = std::chrono::high_resolution_clock::now();
    command = "draco_decoder -i " + com_path + " -o " + deco_path;
    system(command.c_str());
    end = std::chrono::high_resolution_clock::now();

    DrawableTrimesh<> m_decompressa(std::string(deco_path).c_str());
    ris.sizeDecompressed = fs::directory_entry(deco_path).file_size();
    ris.vertDecompressed = m_decompressa.num_verts();
    ris.edgesDecompressed = m_decompressa.num_edges();
    ris.polyDecompressed = m_decompressa.num_polys();

    elapsed = end-start;
    ris.decompressionTime = elapsed.count();

    //calcolo percentuale di perdita di informazioni

    ris.sizeLoss = (double(ris.sizeCompressed)*100)/double(ris.sizeOriginal);
    ris.vertLoss = 100.0 - ((double)ris.vertDecompressed * 100 / (double)ris.vertOriginal);
    ris.edgesLoss = 100.0 - ((double)ris.edgesDecompressed * 100 / (double)ris.edgesOriginal);
    ris.polyLoss = 100.0 - ((double)ris.polyDecompressed * 100 / (double)ris.polyOriginal);


    //todo modificare intestazione file


    writeToFile(csv_path, ris);


    //Pulizia della cartella temporanea
    //fs::remove_all(tempDir);

    // Verifica se il file esiste prima di tentare di rimuoverlo
    if (fs::exists(com_path)) {
        try {
            fs::remove(com_path); // Rimuove il file
            std::cout << "File rimosso con successo." << std::endl;
        } catch (const std::filesystem::filesystem_error& err) {
            std::cerr << "Errore durante la rimozione del file: " << err.what() << std::endl;
        }
    }
    // Verifica se il file esiste prima di tentare di rimuoverlo
    if (fs::exists(deco_path)) {
        try {
            fs::remove(deco_path); // Rimuove il file
            std::cout << "File rimosso con successo." << std::endl;
        } catch (const std::filesystem::filesystem_error& err) {
            std::cerr << "Errore durante la rimozione del file: " << err.what() << std::endl;
        }
    }

    return 0;
}

