#include "dosermodel.h"

DoserModel::DoserModel()
{

}

void DoserModel::openImage(QString path)
{
	QImage newImage(path);
	if (!newImage.isNull())
	{
		image = newImage;
		emit imageChanged(image);
	}
}
