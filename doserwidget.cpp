#include "doserwidget.h"
#include <QComboBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QImage>
#include <QLabel>
#include <QLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QString>
#include <QTime>
#include <QVBoxLayout>

DoserWidget::DoserWidget(QWidget *parent) : QWidget(parent)
{
	setupModel();
	setupGui();
	qsrand(QTime::currentTime().msec());
}

DoserWidget::~DoserWidget()
{
	modelThread.quit();
	modelThread.wait();
}

void DoserWidget::changeGuiMode()
{
	SegmentationMode mode = currentMode();
	bool isQuickVisible = mode == QUICK_MODE || mode == BOTH_MODE;
	bool isDeepVisible = mode == DEEP_MODE || mode == BOTH_MODE;

	displayGridColumn(QUICK_GROUP_COLUMN_INDEX, isQuickVisible);
	displayGridColumn(DEEP_GROUP_COLUMN_INDEX, isDeepVisible);
}

void DoserWidget::drawSegment(const DoserModel::Segment& segment)
{
	QColor randomColor = QColor(qrand() % 255, qrand() % 255, qrand() % 255);

	for (const DoserModel::Pixel& p : segment)
	{
		deepImage.setPixelColor(p, randomColor);
	}

	imageLabels[DEEP_LABEL]->setPixmap(QPixmap::fromImage(deepImage));
	subProgressBar->setValue(0);
}

void DoserWidget::imageChanged(const QImage& image)
{
	sourceImage = image;

	imageLabels[SOURCE_LABEL]->setPixmap(QPixmap::fromImage(sourceImage));
	imageLabels[DEEP_LABEL]->setText("Deep segments\nnot yet computed.");

	emit status("Image successfully opened.");
	setButtonsEnabled(true);
}

void DoserWidget::iterationProgressChanged(int current, int max)
{
	subProgressBar->setValue(current * 100 / max);
}

void DoserWidget::openImage()
{
	QString imagePath = QFileDialog::getOpenFileName(this,
		tr("Open original image"), "", tr("Image files (*.png *.jpg *.bmp)"));
	if (!imagePath.isEmpty() && !imagePath.isNull())
	{
		setButtonsEnabled(false);
		emit status("Opening image...");
		emit doOpenImage(imagePath);
	}
}

void DoserWidget::segment()
{
	setButtonsEnabled(false);
	emit status("Segmenting image...");

	deepImage = sourceImage;
	imageLabels[DEEP_LABEL]->setPixmap(QPixmap::fromImage(deepImage));

	mainProgressBar->setFormat("Total segmentation: %p%");
	subProgressBar->setFormat("Current iteration: %p%");

	emit doSegment(currentMode());
}

void DoserWidget::segmentationFinished()
{
	mainProgressBar->setValue(0);
	mainProgressBar->setFormat("Total segmentation");
	subProgressBar->setFormat("Current iteration");

	emit status("Image successfully segmented.");
	setButtonsEnabled(true);
}

void DoserWidget::segmentationProgressChanged(int current, int max)
{
	mainProgressBar->setValue(current * 100 / max);
}

SegmentationMode DoserWidget::currentMode() const
{
	return static_cast<SegmentationMode>(modeComboBox->currentData().toInt());
}

void DoserWidget::setButtonsEnabled(bool enabled)
{
	segmentButton->setEnabled(enabled && !sourceImage.isNull());
	openButton->setEnabled(enabled);
	saveQuickButton->setEnabled(enabled);
	saveDeepButton->setEnabled(enabled);
}

void DoserWidget::setupModel()
{
	model = new DoserModel;
	model->moveToThread(&modelThread);

	qRegisterMetaType<QVector<QPoint>>("QVector<QPoint>");
	qRegisterMetaType<SegmentationMode>("SegmentationMode");

	connect(&modelThread, SIGNAL(finished()), model, SLOT(deleteLater()));
	connect(this, SIGNAL(doOpenImage(QString)), model, SLOT(openImage(QString)));
	connect(model, SIGNAL(imageChanged(const QImage&)), this, SLOT(imageChanged(const QImage&)));
	connect(this, SIGNAL(doSegment(SegmentationMode)), model, SLOT(segment(SegmentationMode)));
	connect(model, SIGNAL(deepSegmentChanged(DoserModel::Segment)), this, SLOT(drawSegment(DoserModel::Segment)));
	connect(model, SIGNAL(iterationProgress(int,int)), this, SLOT(iterationProgressChanged(int,int)));
	connect(model, SIGNAL(segmentationFinished()), this, SLOT(segmentationFinished()));
	connect(model, SIGNAL(segmentationProgress(int,int)), this, SLOT(segmentationProgressChanged(int,int)));

	modelThread.start();
}

void DoserWidget::setupGui()
{
	QVector<QGroupBox*> groups = createGuiGroups();

	gridLayout = new QGridLayout;
	gridLayout->addWidget(groups[0], 0, 0);
	gridLayout->addWidget(groups[1], 0, 1);
	gridLayout->addWidget(groups[2], 0, 2);
	gridLayout->addWidget(groups[3], 0, 3);

	segmentButton = new QPushButton("Segmentation");
	connect(segmentButton, SIGNAL(clicked(bool)), this, SLOT(segment()));

	openButton = new QPushButton("Open");
	connect(openButton, SIGNAL(clicked(bool)), this, SLOT(openImage()));

	saveQuickButton = new QPushButton("Save");
	saveDeepButton = new QPushButton("Save");

	gridLayout->addWidget(openButton, 1, 1);
	gridLayout->addWidget(segmentButton, 1, 0);
	gridLayout->addWidget(saveQuickButton, 1, 2);
	gridLayout->addWidget(saveDeepButton, 1, 3);

	mainProgressBar = new QProgressBar;
	subProgressBar = new QProgressBar;

	const static auto& initProgressBar = [](QProgressBar* bar)
	{
		bar->setRange(0, 100);
		bar->setValue(0);
		bar->setFixedHeight(18);
	};

	initProgressBar(mainProgressBar);
	initProgressBar(subProgressBar);
	mainProgressBar->setFormat("Total segmentation");
	subProgressBar->setFormat("Current iteration");

	QVBoxLayout* progressLayout = new QVBoxLayout;
	progressLayout->addSpacing(5);
	progressLayout->addWidget(subProgressBar);
	progressLayout->addWidget(mainProgressBar);

	QGroupBox* progressGroup = new QGroupBox("Segmentation progress");
	progressGroup->setLayout(progressLayout);

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addLayout(gridLayout);
	mainLayout->addWidget(progressGroup);
	setLayout(mainLayout);

	changeGuiMode();
	setButtonsEnabled(true);
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
	modeComboBox->setCurrentIndex(1);
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
		label->setScaledContents(true);
		imageLabels.insert(labelTypes[i], label);

		QVBoxLayout* layout = new QVBoxLayout;
		layout->addWidget(label);

		QGroupBox* group = new QGroupBox(groupTitles[i]);
		group->setLayout(layout);
		groups.push_back(group);
	}

	return groups;
}
