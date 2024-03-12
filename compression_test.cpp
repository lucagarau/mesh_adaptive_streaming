#include <cinolib/meshes/drawable_trimesh.h>
#include <cinolib/gl/surface_mesh_controls.h>
#include <cinolib/gl/glcanvas.h>
#include <filesystem>
#include <iostream>
#include <fstream>

//#define INPUT_PATH R"(C:\Users\luca-\OneDrive\Data\progetti\Borsa\lib\common-3d-test-models\data)"
#define INPUT_PATH R"(C:\Users\luca-\OneDrive\Data\progetti\Borsa\test_compressione\meshes\test)"
#define OUTPUT_PATH R"(C:\Users\luca-\OneDrive\Data\progetti\Borsa\test_compressione\meshes\compressed)"
#define DECO_PATH R"(C:\Users\luca-\OneDrive\Data\progetti\Borsa\test_compressione\meshes\decompressed)"
#define RESULTS_PATH R"(C:\Users\luca-\OneDrive\Data\progetti\Borsa\test_compressione\results\)"

#define MESH_EXTENSION ".obj"
#define OUTPUT_EXTENSION ".drc"

//compression level, quanto più è alto più è compresso, valore tra 0 e 10
#define CL 10

//quantization bits, quanto più è alto più è compresso,valore tra 0 e 30
#define QP 13


namespace fs = std::filesystem;


struct results{
    //file originale
    std::string model = "";
    uint sizeOriginal = 0;
    uint vertOriginal = 0;
    uint edgesOriginal = 0;
    uint polyOriginal = 0;

    //file compresso
    uint sizeCompressed = 0;
    int cl = CL;
    int qp= QP;

    //file decompresso
    uint sizeDecompressed = 0;
    uint vertDecompressed = 0;
    uint edgesDecompressed = 0;
    uint polyDecompressed = 0;

    //percentuali di perdita di dati
    double compressionRatio = 0.0;
    double vertLoss = 0.0;
    double edgesLoss = 0.0;
    double polyLoss = 0.0;

};

int main()
{
    using namespace cinolib;
    using namespace std::chrono;
    using namespace std;


    //dichiarazione variabili di supporto
    std::string in_path = std::string(INPUT_PATH);
    std::string out_path = std::string(OUTPUT_PATH);
    std::string deco_path = std::string(DECO_PATH);
    std::string fileCompresso;
    std::string fileOriginale;
    std::string command;


    std::vector<results> resultsTable;


    int k = 0;
    cout << "inizio test..." << endl;
    //leggo tutte le mesh
    for (const auto& entry : fs::directory_iterator(in_path)) {
        if (fs::is_regular_file(entry.path()) && entry.path().extension() == MESH_EXTENSION) {

            std::cout << "Compressione di : " << fileOriginale <<" CL = " << std::to_string(CL) <<" QP " << std::to_string(QP) << std::endl;

            fileOriginale = entry.path().filename().string();
            fileCompresso = entry.path().stem().string() + std::string(OUTPUT_EXTENSION);
            results ris;

            //lettura modello originale
            DrawableTrimesh<> m_originale(entry.path().string().c_str());
            ris.model = fileOriginale;
            ris.sizeOriginal = entry.file_size();
            ris.vertOriginal = m_originale.num_verts();
            ris.edgesOriginal = m_originale.num_edges();
            ris.polyOriginal = m_originale.num_polys();

            //compressione del modello
            command = "draco_encoder -i " + in_path +"\\" +fileOriginale + " -o " + out_path +"\\"+fileCompresso+ " -cl " + std::to_string(CL) + " -qp " + std::to_string(QP);
            system(command.c_str());
            ris.sizeCompressed = fs::directory_entry(out_path + "\\" + fileCompresso).file_size();
            ris.cl = CL;
            ris.qp = QP;

            //decompressione del modello
            command = "draco_decoder -i " + out_path +"\\" +fileCompresso + " -o " + deco_path +"\\"+ entry.path().filename().string();
            system(command.c_str());
            DrawableTrimesh<> m_decompressa(std::string(deco_path + "\\" + fileOriginale).c_str());
            ris.sizeDecompressed = fs::directory_entry(deco_path + "\\" + fileOriginale).file_size();
            ris.vertDecompressed = m_decompressa.num_verts();
            ris.edgesDecompressed = m_decompressa.num_edges();
            ris.polyDecompressed = m_decompressa.num_polys();

            //calcolo percentuale di perdita di informazioni

            ris.compressionRatio = double(ris.sizeOriginal) / double(ris.sizeCompressed);
            ris.vertLoss = (double (ris.vertOriginal- ris.vertDecompressed)*100)/double(ris.vertOriginal);
            ris.edgesLoss = (double (ris.edgesOriginal- ris.edgesDecompressed)*100)/double(ris.edgesOriginal);
            ris.polyLoss = (double (ris.polyOriginal- ris.polyDecompressed)*100)/double(ris.polyOriginal);
            
            
            resultsTable.push_back(ris);
            k++;
            cout << "Iteranione numero: " << k << endl;
        }

    }
    cout << "Test completato" << endl;
    cout << "Salvataggio dei risultati..." << endl;
    // Ottenere il timestamp corrente
    auto now = system_clock::now();
    time_t timestamp = system_clock::to_time_t(now);

    // Formatta il timestamp come stringa
    stringstream ss;
    ss << put_time(localtime(&timestamp), "%y%m%d%H%M");
    string timestampStr = ss.str();

    // Nome del file CSV basato sul timestamp
    string filename = string(RESULTS_PATH)+"results_cl_"+ to_string(CL)+"_qp_"+ to_string(QP)+"_" + timestampStr + ".csv";

    //salvataggio in un file csv
    std::ofstream file;
    file.open(filename);

    file << "Model;SizeOriginal;VertOriginal;EdgesOriginal;PolyOriginal;SizeCompressed;CL;QP;SizeDecompressed;VertDecompressed;EdgesDecompressed;PolyDecompressed;compressionRatio;VertLoss;EdgesLoss;PolyLoss\n";
    for (auto r : resultsTable){
        file << r.model << ";" << r.sizeOriginal << ";" << r.vertOriginal << ";" << r.edgesOriginal << ";" << r.polyOriginal << ";" << r.sizeCompressed << ";" << r.cl << ";" << r.qp << ";" << r.sizeDecompressed << ";" << r.vertDecompressed << ";" << r.edgesDecompressed << ";" << r.polyDecompressed << ";" << r.compressionRatio << ";" << r.vertLoss << ";" << r.edgesLoss << ";" << r.polyLoss << "\n";
    }
    file.close();

    cout << "Risultati salvati in: " << filename << endl;

    return 0;
}
