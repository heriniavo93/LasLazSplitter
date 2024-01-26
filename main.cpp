#include <thread>
#include <iostream>
#include <pdal/Stage.hpp>
#include <pdal/StageFactory.hpp>
#include <pdal/util/FileUtils.hpp>
#include <io/LasWriter.hpp>
#include <filters/StreamCallbackFilter.hpp>
#include <mutex>
#include "Timer.h"

// Récupérer les variables globales
std::mutex mutex_;
std::string file_;
std::string output_ = "auto";
unsigned long cloud_size_ = 0;
bool monothread_ = false;
int chunk_size_ = 5000000;


std::string joinPath(std::string dire, std::string filename) {
	while (dire.back() == '\\' || dire.back() == '/')
		dire.pop_back();
	while (filename.back() == '\\' || filename.back() == '/')
		filename.erase(0, 1);
	return dire + "\\" + filename;
}


void streamBetween(size_t start, size_t end, size_t file_number) {
	// Créer le dossier et le fichier correspondant
	std::string filename = pdal::FileUtils::stem(file_);

	// Chemin du fichier de sortie
	filename = filename + "_" + std::to_string(file_number) + ".laz";
	std::string output = joinPath(output_, filename);

	// Protégér l'output stream
	mutex_.lock();
	std::cout << std::fixed << "Enregistrement du fichier: " << pdal::FileUtils::getFilename(output) << " (" << std::to_string(end - start) << " Points)" << std::endl;
	mutex_.unlock();

	// Créer le reader
	pdal::Options opt;
	opt.add("start", start);
	opt.add("count", end - start);
	opt.add("filename", file_);
	pdal::StageFactory factory;
	pdal::Stage * reader = factory.createStage("readers.las");
	reader->setOptions(opt);
	pdal::PointTable table;
	reader->prepare(table);

	// Créer le writer
	pdal::Options wOpts;
	wOpts.add("filename", output);
	wOpts.add("offset_x", "auto");
	wOpts.add("offset_y", "auto");
	wOpts.add("offset_z", 0);
	wOpts.add("scale_x", "auto");
	wOpts.add("scale_y", "auto");
	wOpts.add("scale_z", "auto");

	// Créer le manager
	pdal::PipelineManager mgr;
	try
	{
		auto writer = &mgr.addWriter("writers.las");
		writer->setInput(*reader);
		writer->setOptions(wOpts);
		mgr.prepare();
		mgr.execute(pdal::ExecMode::PreferStream);
	}
	catch (const pdal::pdal_error& e)
	{
		std::cout << e.what() << std::endl;
	}
}


void compute() {
	// Récupérer le nombre maximale de threads qu'on pourrais utiliser
	unsigned processorCount = std::thread::hardware_concurrency();
	if (processorCount == 0 || monothread_)
		processorCount = 1;

	std::vector<std::thread*> threads;

	// Déclarer les variables necessaires au calcul
	size_t start = 0;
	size_t end = 0;
	bool begin = true;

	// Diviser les tâches en plusieurs threads
	int count = 1;
	const size_t step = std::ceil((double)cloud_size_ / (double)chunk_size_);
	for (int part = 0; part < step; part = part + processorCount) {
		bool break_here = false;
		for (unsigned i = 0; i < processorCount; ++i)
		{
			if (break_here || start > cloud_size_)
				break;

			// Récupérer le start et le end
			if (begin) {
				begin = false;
				start = i * chunk_size_;
			}
			else
				start = end + 1;
			end = start + chunk_size_;

			if (end > cloud_size_) {
				end = cloud_size_;
				break_here = true;
			}

			// Remplir les threads
			threads.push_back(new std::thread(&streamBetween, start, end, count));
			count += 1;
		}

		// Lancer les threads
		for (auto it = threads.begin(); it != threads.end(); ++it)
			(*it)->join();

		for (auto it = threads.begin(); it != threads.end(); ++it)
			delete (*it);
	}
}

void help() {
	std::cout << "Programme permettant de découper un fichier las ou laz en petits morceaux." << std::endl;
	std::cout << "Liste des paramètres:" << std::endl;
	std::cout << "\t- Chemin du fichier las ou laz à traiter" << std::endl;
	std::cout << "\t- Chemin du dossier de sortie" << std::endl;
	std::cout << "\t- Nombre de points par morceaux de fichier. Optionnel, par défaut 5000000" << std::endl;
	std::cout << "\t- Utiliser le multithread. Par défaut à true" << std::endl;
	std::cout << "Exemple: c:\\path\\to\\my\\laz\\file.las D:\\path\\to\\output\\dir 1000000 false" << std::endl;
}

int main(int argc, char* argv[])
{
	// Gérér les accents
	setlocale(LC_ALL, "");
	if (argc < 3)
	{
		std::cout << "Argument insuffisant" << std::endl;
		help();
		return 1;
	}

	// Récupérer les paramètres d'entrées
	file_ = argv[1];
	output_ = argv[2];

	// Créer le dossier de sortie
	std::string filename = pdal::FileUtils::stem(file_);
	output_ = joinPath(output_, filename);
	if (!pdal::FileUtils::directoryExists(output_)) {
		// Créer le dossier
		std::cout << output_ << std::endl;
		if (!pdal::FileUtils::createDirectories(output_)) {
			std::cout << "\t-Impossible de créer le dossier de sortie" << std::endl;
			return 1;
		}
	}

	monothread_ = false;
	if (argc >= 4) {
		try
		{
			// Récupérer la taille d'un morceau
			chunk_size_ = std::stoul(argv[3]);
		}

		catch (...)
		{
			help();
			return 1;
		}

		// Vérifier si on a encore d'autres paramètres
		if (argc == 5) {
			if (argv[4] == "false")
				monothread_ = true;
		}
	}

	// MULTITHREAD
	{
		// Déclarer un timer
		Timer duration("Temps d'exécution de la conversion");
		pdal::StageFactory factory;
		pdal::Stage * reader = factory.createStage("readers.las");
		if (!reader)
			return 1;

		pdal::Options options_;
		options_.add("filename", file_);
		reader->setOptions(options_);

		try
		{
			pdal::PointTable table;
			reader->prepare(table);

			auto previ = reader->preview();
			cloud_size_ = previ.m_pointCount;

			// Lancer la commande
			compute();
		}
		catch (const pdal::pdal_error & e)
		{
			std::cout << "\tErreur:" << e.what() << std::endl;
		}

	}
	return 0;
}