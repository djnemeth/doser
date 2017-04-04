#include "dosermodel.h"

DoserModel::DoserModel()
{

}

void DoserModel::openImage(const QString& path)
{
	QImage newImage(path);
	if (!newImage.isNull())
	{
		image = newImage;

		// some busy wait for testing
		clock_t start = clock();
		while (((float)(clock() - start)) / CLOCKS_PER_SEC < 3);

		emit imageChanged(image);
	}
}
