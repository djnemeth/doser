#include "doserwidget.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QImage>
#include <QLabel>
#include <QLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QString>
#include <QTime>
#include <QtMath>
#include <QVBoxLayout>

// constructor and destructor

DoserWidget::DoserWidget(QWidget* parent) : QWidget(parent)
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

// handlers of model events

void DoserWidget::imageChanged(const QImage& image)
{
	images[SOURCE] = image;
	imageLabels[SOURCE]->setPixmap(QPixmap::fromImage(images[SOURCE]));
	resetImages();

	emit status("Image successfully opened.");
	setControlsEnabled(true);
}

void DoserWidget::segmentationStarted(DoserModel::SegmentationMode mode)
{
	setControlsEnabled(false);

	GuiElementType type = toGuiElementType(mode);
	images[type] = images[SOURCE];
	imageLabels[type]->setPixmap(QPixmap::fromImage(images[type]));

	emit status(toString(mode) + " segmenting image...");
	mainProgressBar->setFormat("Total segmentation: %p%");
}

void DoserWidget::drawSegment(DoserModel::SegmentationMode mode, const DoserModel::Segment& segment)
{
	GuiElementType type = toGuiElementType(mode);
	QColor randomColor = QColor(qrand() % 255, qrand() % 255, qrand() % 255);

	for (const DoserModel::Pixel& p : segment)
	{
		images[type].setPixelColor(p, randomColor);
	}

	imageLabels[type]->setPixmap(QPixmap::fromImage(images[type]));
}

void DoserWidget::segmentationFinished(DoserModel::SegmentationMode mode, const QVector<DoserModel::Segment>& finalSegments)
{
	for (const DoserModel::Segment& segment : finalSegments)
	{
		drawSegment(mode, segment);
	}

	mainProgressBar->setValue(0);
	mainProgressBar->setFormat("Total segmentation");
	subProgressBar->setValue(0);
	subProgressBar->setFormat("Current subprocess");

	emit status("Image successfully segmented.");
	setControlsEnabled(true);
}

void DoserWidget::segmentationProgressChanged(int current, int max)
{
	mainProgressBar->setValue(current * 100 / max);
}

void DoserWidget::subProcessProgressChanged(DoserModel::SubProcessType type, int current, int max)
{
	subProgressBar->setFormat("Current " + toString(type) + ": %p%");
	subProgressBar->setValue(current * 100 / max);
}

// utility slots

void DoserWidget::changeGuiMode()
{
	DoserModel::SegmentationMode mode = currentMode();
	bool isQuickVisible = mode == DoserModel::QUICK_MODE || mode == DoserModel::BOTH_MODE;
	bool isDeepVisible = mode == DoserModel::DEEP_MODE || mode == DoserModel::BOTH_MODE;

	samplingProbabilitySpin->setEnabled(isQuickVisible);
	displayGridColumn(QUICK_GROUP_COLUMN_INDEX, isQuickVisible);
	displayGridColumn(DEEP_GROUP_COLUMN_INDEX, isDeepVisible);
}

void DoserWidget::openImage()
{
	QString imagePath = QFileDialog::getOpenFileName(this,
		tr("Open original image"), "", tr("Image files (*.png *.jpg *.bmp)"));
	if (!imagePath.isEmpty() && !imagePath.isNull())
	{
		setControlsEnabled(false);
		emit status("Opening image...");
		emit doOpenImage(imagePath);
	}
}

void DoserWidget::segment()
{
	resetImages();

	DoserModel::SegmentationParameters parameters;
	parameters.targetSegmentationRatio = targetSegmentationRatioSpin->value() / 100.0;
	parameters.minimalSegmentSize = minimalSegmentSizeSpin->value();
	parameters.iterationPrecision = iterationPrecisionSpin->value();
	parameters.samplingProbability = samplingProbabilitySpin->value() / 100.0;
	parameters.weightRatioSquare = qPow(weightRatioSpin->value(), 2);
	parameters.forceGrayscale = forceGrayscaleCheckBox->isChecked();

	emit doSegment(currentMode(), parameters);
}

// initializer procedures

void DoserWidget::setupModel()
{
	model = new DoserModel;

	// thread-related
	model->moveToThread(&modelThread);
	connect(&modelThread, SIGNAL(finished()), model, SLOT(deleteLater()));

	// image-related
	connect(this, SIGNAL(doOpenImage(QString)), model, SLOT(openImage(QString)));
	connect(model, SIGNAL(imageChanged(QImage)),
		this, SLOT(imageChanged(QImage)));

	// segmentation-related
	connect(this, SIGNAL(doSegment(DoserModel::SegmentationMode, DoserModel::SegmentationParameters)),
		model, SLOT(segment(DoserModel::SegmentationMode, DoserModel::SegmentationParameters)));
	connect(model, SIGNAL(segmentationStarted(DoserModel::SegmentationMode)),
		this, SLOT(segmentationStarted(DoserModel::SegmentationMode)));
	connect(model, SIGNAL(segmentChanged(DoserModel::SegmentationMode, DoserModel::Segment)),
		this, SLOT(drawSegment(DoserModel::SegmentationMode, DoserModel::Segment)));
	connect(model, SIGNAL(segmentationFinished(DoserModel::SegmentationMode, QVector<DoserModel::Segment>)),
		this, SLOT(segmentationFinished(DoserModel::SegmentationMode, QVector<DoserModel::Segment>)));

	// progress-related
	connect(model, SIGNAL(segmentationProgress(int, int)),
		this, SLOT(segmentationProgressChanged(int, int)));
	connect(model, SIGNAL(subProcessProgress(DoserModel::SubProcessType, int, int)),
		this, SLOT(subProcessProgressChanged(DoserModel::SubProcessType, int, int)));

	modelThread.start();
}

void DoserWidget::setupGui()
{
	// groups

	QVector<QGroupBox*> groups = createGuiGroups();

	gridLayout = new QGridLayout;
	gridLayout->addWidget(groups[0], 0, 0);
	gridLayout->addWidget(groups[1], 0, 1);
	gridLayout->addWidget(groups[2], 0, 2);
	gridLayout->addWidget(groups[3], 0, 3);

	// controls

	segmentButton = new QPushButton("Segmentation");
	connect(segmentButton, SIGNAL(clicked(bool)), this, SLOT(segment()));

	openButton = new QPushButton("Open");
	connect(openButton, SIGNAL(clicked(bool)), this, SLOT(openImage()));

	saveButtons[QUICK] = new QPushButton("Save");
	saveButtons[DEEP] = new QPushButton("Save");

	gridLayout->addWidget(openButton, 1, 1);
	gridLayout->addWidget(segmentButton, 1, 0);
	gridLayout->addWidget(saveButtons[DEEP], 1, DEEP_GROUP_COLUMN_INDEX);
	gridLayout->addWidget(saveButtons[QUICK], 1, QUICK_GROUP_COLUMN_INDEX);

	// progress tracking

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
	subProgressBar->setFormat("Current subprocess");

	QVBoxLayout* progressLayout = new QVBoxLayout;
	progressLayout->addSpacing(5);
	progressLayout->addWidget(subProgressBar);
	progressLayout->addWidget(mainProgressBar);

	QGroupBox* progressGroup = new QGroupBox("Segmentation progress");
	progressGroup->setLayout(progressLayout);

	// assembling

	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addLayout(gridLayout);
	mainLayout->addWidget(progressGroup);
	setLayout(mainLayout);

	changeGuiMode();
	setControlsEnabled(true);
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

	QVector<GuiElementType> guiElementTypes = { SOURCE, DEEP, QUICK };
	QVector<QString> groupTitles = { "Original image", "Deep segments", "Quick segments" };
	QVector<QString> labelTexts =
	{
		"Original image\nnot yet selected.",
		"Deep segments\nnot yet computed.",
		"Quick segments\nnot yet computed."
	};

	for (int i = 0; i < guiElementTypes.size(); ++i)
	{
		QLabel* label = new QLabel(labelTexts[i]);
		label->setAlignment(Qt::AlignCenter);
		label->setScaledContents(true);
		imageLabels.insert(guiElementTypes[i], label);

		QVBoxLayout* layout = new QVBoxLayout;
		layout->addWidget(label);

		QGroupBox* group = new QGroupBox(groupTitles[i]);
		group->setLayout(layout);
		groups.push_back(group);
	}

	return groups;
}

QGroupBox* DoserWidget::createSettingsGui()
{
	// mode

	modeComboBox = new QComboBox;
	modeComboBox->addItem("quick", DoserModel::QUICK_MODE);
	modeComboBox->addItem("deep", DoserModel::DEEP_MODE);
	modeComboBox->addItem("deep & quick", DoserModel::BOTH_MODE);
	connect(modeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(changeGuiMode()));

	// target ratio

	targetSegmentationRatioSpin = new QSpinBox;
	targetSegmentationRatioSpin->setRange(0, 100);
	targetSegmentationRatioSpin->setSingleStep(1);
	targetSegmentationRatioSpin->setSuffix("%");
	targetSegmentationRatioSpin->setValue(90);

	// minimal segment size

	minimalSegmentSizeSpin = new QSpinBox;
	minimalSegmentSizeSpin->setMinimum(0);
	minimalSegmentSizeSpin->setSingleStep(1);
	minimalSegmentSizeSpin->setSuffix("px");
	minimalSegmentSizeSpin->setValue(50);

	// iteration precision

	iterationPrecisionSpin = new QDoubleSpinBox;
	iterationPrecisionSpin->setMinimum(0);
	iterationPrecisionSpin->setSingleStep(0.01);
	iterationPrecisionSpin->setValue(0.01);

	// sampling probability

	samplingProbabilitySpin = new QDoubleSpinBox;
	samplingProbabilitySpin->setRange(0, 100);
	samplingProbabilitySpin->setSingleStep(0.5);
	samplingProbabilitySpin->setDecimals(1);
	samplingProbabilitySpin->setSuffix("%");
	samplingProbabilitySpin->setValue(10);

	// weight ratio

	weightRatioSpin = new QDoubleSpinBox;
	weightRatioSpin->setMinimum(0.01);
	weightRatioSpin->setSingleStep(0.01);
	weightRatioSpin->setValue(2);

	// force grayscale

	forceGrayscaleCheckBox = new QCheckBox;
	forceGrayscaleCheckBox->setChecked(false);

	// assembly

	QGridLayout* settingsLayout = new QGridLayout;
	settingsLayout->addWidget(new QLabel("Mode:"), 0, 0);
	settingsLayout->addWidget(modeComboBox, 0, 1);
	settingsLayout->addWidget(new QLabel("Target ratio:"), 1, 0);
	settingsLayout->addWidget(targetSegmentationRatioSpin, 1, 1);
	settingsLayout->addWidget(new QLabel("Minimal size:"), 2, 0);
	settingsLayout->addWidget(minimalSegmentSizeSpin, 2, 1);
	settingsLayout->addWidget(new QLabel("Precision:"), 3, 0);
	settingsLayout->addWidget(iterationPrecisionSpin, 3, 1);
	settingsLayout->addWidget(new QLabel("Sampling ratio:"), 4, 0);
	settingsLayout->addWidget(samplingProbabilitySpin, 4, 1);
	settingsLayout->addWidget(new QLabel("Weight ratio:"), 5, 0);
	settingsLayout->addWidget(weightRatioSpin, 5, 1);
	settingsLayout->addWidget(new QLabel("Force grayscale:"), 6, 0);
	settingsLayout->addWidget(forceGrayscaleCheckBox, 6, 1);
	settingsLayout->setRowStretch(7, 1);

	QGroupBox* settingsGroup = new QGroupBox("Settings");
	settingsGroup->setLayout(settingsLayout);
	return settingsGroup;
}

// gui-altering procedures

void DoserWidget::displayGridColumn(int column, bool isVisible)
{
	for (int i = 0; i < gridLayout->rowCount(); ++i)
	{
		gridLayout->itemAtPosition(i, column)->widget()->setVisible(isVisible);
	}
}

void DoserWidget::resetImages()
{
	images[QUICK] = QImage();
	images[DEEP] = QImage();
	imageLabels[QUICK]->setText("Quick segments\nnot yet computed.");
	imageLabels[DEEP]->setText("Deep segments\nnot yet computed.");
}

void DoserWidget::setControlsEnabled(bool enabled)
{
	modeComboBox->setEnabled(enabled);
	targetSegmentationRatioSpin->setEnabled(enabled);
	minimalSegmentSizeSpin->setEnabled(enabled);
	iterationPrecisionSpin->setEnabled(enabled);
	samplingProbabilitySpin->setEnabled((currentMode() == DoserModel::QUICK_MODE
		|| currentMode() == DoserModel::BOTH_MODE) && enabled);
	weightRatioSpin->setEnabled(enabled);
	forceGrayscaleCheckBox->setEnabled(enabled);

	segmentButton->setEnabled(enabled && !images[SOURCE].isNull());
	openButton->setEnabled(enabled);
	saveButtons[QUICK]->setEnabled(enabled && !images[QUICK].isNull());
	saveButtons[DEEP]->setEnabled(enabled && !images[DEEP].isNull());
}

// utility functions

DoserModel::SegmentationMode DoserWidget::currentMode() const
{
	return static_cast<DoserModel::SegmentationMode>(modeComboBox->currentData().toInt());
}

DoserWidget::GuiElementType DoserWidget::toGuiElementType(DoserModel::SegmentationMode mode) const
{
	if (mode == DoserModel::QUICK_MODE)
	{
		return QUICK;
	}

	return DEEP;
}

QString DoserWidget::toString(DoserModel::SegmentationMode mode) const
{
	switch (mode)
	{
	case DoserModel::QUICK_MODE:
		return "Quick";
	case DoserModel::DEEP_MODE:
		return "Deep";
	default:
		return "";
	}
}

QString DoserWidget::toString(DoserModel::SubProcessType type) const
{
	switch (type)
	{
	case DoserModel::ITERATION:
		return "iteration";
	case DoserModel::EXTRAPOLATION:
		return "extrapolation";
	case DoserModel::MERGING:
		return "merging";
	default:
		return "subprocess";
	}
}
