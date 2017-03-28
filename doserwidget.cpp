#include "doserwidget.h"
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

DoserWidget::DoserWidget(QWidget *parent) : QWidget(parent)
{
	QGridLayout* layout = new QGridLayout;

	QVector<QLabel*> labels =
	{
		new QLabel("Settings"),
		new QLabel("Original image"),
		new QLabel("Result of deep\nsegmentation\n(optional)"),
		new QLabel("Result of quick\nsegmentation\n(optional)")
	};

	for (QLabel* label : labels)
	{
		label->setAlignment(Qt::AlignCenter);
		label->setFrameStyle(QFrame::Panel);
		label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	}

	layout->addWidget(labels[0], 0, 0);
	layout->addWidget(labels[1], 0, 1);
	layout->addWidget(labels[2], 0, 2);
	layout->addWidget(labels[3], 0, 3);

	layout->addWidget(new QPushButton("Segmentation"), 1, 0);
	layout->addWidget(new QPushButton("Open"), 1, 1);
	layout->addWidget(new QPushButton("Save"), 1, 2);
	layout->addWidget(new QPushButton("Save"), 1, 3);

	setLayout(layout);
}
