#include "doserwidget.h"
#include <QComboBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

#include <QDebug>

DoserWidget::DoserWidget(QWidget *parent) : QWidget(parent)
{
	setupModel();
	setupUi();
}

DoserWidget::~DoserWidget()
{
	modelThread.quit();
	modelThread.wait();
}

void DoserWidget::changeGuiMode()
{
	SegmentationMode mode = static_cast<SegmentationMode>(
		modeComboBox->currentData().toInt());

	bool isQuickVisible = mode == QUICK_MODE || mode == BOTH_MODE;
	bool isDeepVisible = mode == DEEP_MODE || mode == BOTH_MODE;

	displayGridColumn(QUICK_GROUP_COLUMN_INDEX, isQuickVisible);
	displayGridColumn(DEEP_GROUP_COLUMN_INDEX, isDeepVisible);
}

void DoserWidget::imageChanged(const QImage& image)
{
	imageLabels[SOURCE_LABEL]->setPixmap(QPixmap::fromImage(image));
	imageLabels[SOURCE_LABEL]->setScaledContents(true);

	mainProgressBar->setMaximum(100);
	emit status("Image successfully opened.");
	openButton->setEnabled(true);
}

void DoserWidget::openImage()
{
	QString imagePath = QFileDialog::getOpenFileName(this,
		tr("Open original image"), "", tr("Image files (*.png *.jpg *.bmp)"));
	if (!imagePath.isEmpty() && !imagePath.isNull())
	{
		openButton->setEnabled(false);
		emit status("Opening image...");
		mainProgressBar->setMaximum(0);

		emit openImage(imagePath);
	}
}

void DoserWidget::setupModel()
{
	model = new DoserModel;
	model->moveToThread(&modelThread);

	connect(&modelThread, SIGNAL(finished()), model, SLOT(deleteLater()));
	connect(model, SIGNAL(imageChanged(const QImage&)), this, SLOT(imageChanged(const QImage&)));
	connect(this, SIGNAL(openImage(QString)), model, SLOT(openImage(QString)));

	modelThread.start();
}

void DoserWidget::setupUi()
{
	QVector<QGroupBox*> groups = createGuiGroups();

	gridLayout = new QGridLayout;
	gridLayout->addWidget(groups[0], 0, 0);
	gridLayout->addWidget(groups[1], 0, 1);
	gridLayout->addWidget(groups[2], 0, 2);
	gridLayout->addWidget(groups[3], 0, 3);

	openButton = new QPushButton("Open");
	connect(openButton, SIGNAL(clicked(bool)), this, SLOT(openImage()));

	gridLayout->addWidget(openButton, 1, 1);
	gridLayout->addWidget(new QPushButton("Segmentation"), 1, 0);
	gridLayout->addWidget(new QPushButton("Save"), 1, 2);
	gridLayout->addWidget(new QPushButton("Save"), 1, 3);

	auto initProgressBar = [](QProgressBar* bar)
	{
		bar->setRange(0, 100);
		bar->setValue(0);
		bar->setFixedHeight(18);
	};

	mainProgressBar = new QProgressBar;
	subProgressBar = new QProgressBar;

	initProgressBar(mainProgressBar);
	initProgressBar(subProgressBar);
	subProgressBar->setVisible(false);

	QVBoxLayout* progressLayout = new QVBoxLayout;
	progressLayout->addSpacing(5);
	progressLayout->addWidget(mainProgressBar);
	progressLayout->addWidget(subProgressBar);

	QGroupBox* progressGroup = new QGroupBox("Progress");
	progressGroup->setLayout(progressLayout);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addLayout(gridLayout);
	mainLayout->addWidget(progressGroup);
	setLayout(mainLayout);

	changeGuiMode();
}

void DoserWidget::displayGridColumn(int column, bool isVisible)
{
	for (int i = 0; i < gridLayout->rowCount(); ++i)
	{
		gridLayout->itemAtPosition(i, column)->widget()->setVisible(isVisible);
	}
}

QGroupBox* DoserWidget::createSettingsGui()
{
	modeComboBox = new QComboBox;
	modeComboBox->addItem("quick", QUICK_MODE);
	modeComboBox->addItem("deep", DEEP_MODE);
	modeComboBox->addItem("deep & quick", BOTH_MODE);
	connect(modeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeGuiMode()));

	QGridLayout* settingsLayout = new QGridLayout;
	settingsLayout->addWidget(new QLabel("Mode:"), 0, 0);
	settingsLayout->addWidget(modeComboBox, 0, 1);
	settingsLayout->setRowStretch(1, 1);

	QGroupBox* settingsGroup = new QGroupBox("Settings");
	settingsGroup->setLayout(settingsLayout);

	return settingsGroup;
}

QVector<QGroupBox*> DoserWidget::createGuiGroups()
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

	QVector<QGroupBox*> groups;
	groups.push_back(createSettingsGui());

	QVector<ImageLabelType> labelTypes = { SOURCE_LABEL, DEEP_LABEL, QUICK_LABEL };
	QVector<QString> groupTitles = { "Original image", "Deep segments", "Quick segments" };
	QVector<QString> labelTexts =
	{
		"Original image\nnot yet selected.",
		"Deep segments\nnot yet computed.",
		"Quick segments\nnot yet computed."
	};

	for (int i = 0; i < labelTypes.size(); ++i)
	{
		QLabel* label = new QLabel(labelTexts[i]);
		label->setAlignment(Qt::AlignCenter);
		imageLabels.insert(labelTypes[i], label);

		QVBoxLayout* layout = new QVBoxLayout;
		layout->addWidget(label);

		QGroupBox* group = new QGroupBox(groupTitles[i]);
		group->setLayout(layout);
		groups.push_back(group);
	}

	return groups;
}
