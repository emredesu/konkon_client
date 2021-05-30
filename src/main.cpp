#include <QtWidgets/QApplication>
#include "mainwidget.h"

int main(int argc, char* argv[]) {
	try {
		QApplication a(argc, argv);
		MainWidget main_widget;
		a.setWindowIcon(QIcon("icon.ico"));

		// Set up exit handler.
		a.connect(&a, &QApplication::aboutToQuit, &main_widget, &MainWidget::exit_handler);

		main_widget.show();

		return a.exec();
	}
	catch (const std::exception& e) {
		std::cerr << "Exception caught in main: " << e.what() << "\n";
	}
	catch (...) {
		std::cerr << "Non-std::exception caught in main." << "\n";
	}
}
