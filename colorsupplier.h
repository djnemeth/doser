#ifndef COLORSUPPLIER_H
#define COLORSUPPLIER_H

#include <QColor>
#include <QTime>
#include <QVector>

class ColorSupplier
{
public:
	ColorSupplier()
	{
		defaultColors =
		{
			QColor(6, 143, 178), QColor(255, 134, 0), QColor(224, 55, 63),
			QColor(120, 120, 120), QColor(64, 251, 25)
		};

		reset();
	}

	QColor nextColor()
	{
		if (currentColors.isEmpty())
		{
			return QColor(qrand() % 255, qrand() % 255, qrand() % 255);
		}
		else
		{
			return currentColors.takeFirst();
		}
	}

	void reset()
	{
		currentColors = defaultColors;
		qsrand(24);
	}

private:
	QVector<QColor> defaultColors;
	QVector<QColor> currentColors;
};

#endif // COLORSUPPLIER_H
