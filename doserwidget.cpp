#include "doserwidget.h"
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>

DoserWidget::DoserWidget(QWidget *parent) : QWidget(parent)
{
	setStyleSheet("QGroupBox {"
			"border: 1px solid gray;"
			"border-radius: 3px;"
			"margin-top: 1ex;"
		"}"
		"QGroupBox::title {"
			"subcontrol-origin: margin;"
			"left: 1ex;"
		"}");

	QVector<QLabel*> labels =
	{
		new QLabel("Original image"),
		new QLabel("Result of deep\nsegmentation"),
		new QLabel("Result of quick\nsegmentation")
	};

	for (QLabel* label : labels)
	{
		label->setAlignment(Qt::AlignCenter);
		label->setFrameStyle(QFrame::Panel);
		label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	}

	mainLayout = new QGridLayout;
	mainLayout->addWidget(createSettingsGui(), 0, 0);
	mainLayout->addWidget(labels[0], 0, 1);
	mainLayout->addWidget(labels[1], 0, 2);
	mainLayout->addWidget(labels[2], 0, 3);

	mainLayout->addWidget(new QPushButton("Segmentation"), 1, 0);
	mainLayout->addWidget(new QPushButton("Open"), 1, 1);
	mainLayout->addWidget(new QPushButton("Save"), 1, 2);
	mainLayout->addWidget(new QPushButton("Save"), 1, 3);

	setLayout(mainLayout);
	modeChanged();
}

void DoserWidget::modeChanged()
{
	SegmentationMode mode = static_cast<SegmentationMode>(
		modeComboBox->currentData().toInt());

	bool isQuickVisible = mode == QUICK || mode == BOTH;
	bool isDeepVisible = mode == DEEP || mode == BOTH;

	displayGridColumn(QUICK_COLUMN_INDEX, isQuickVisible);
	displayGridColumn(DEEP_COLUMN_INDEX, isDeepVisible);
}

void DoserWidget::displayGridColumn(int column, bool isVisible)
{
	for (int i = 0; i < mainLayout->rowCount(); ++i) {
		mainLayout->itemAtPosition(i, column)->widget()->setVisible(isVisible);
	}
}

QGroupBox* DoserWidget::createSettingsGui()
{
	modeComboBox = new QComboBox;
	modeComboBox->addItem("quick", QUICK);
	modeComboBox->addItem("deep", DEEP);
	modeComboBox->addItem("deep & quick", BOTH);
	connect(modeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(modeChanged()));

	QGridLayout* settingsLayout = new QGridLayout;
	settingsLayout->addWidget(new QLabel("Mode:"), 0, 0);
	settingsLayout->addWidget(modeComboBox, 0, 1);
	settingsLayout->setRowStretch(1, 1);

	QGroupBox* settingsGroup = new QGroupBox("Settings");
	settingsGroup->setLayout(settingsLayout);

	return settingsGroup;
}
