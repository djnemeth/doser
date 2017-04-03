#include "doserwidget.h"
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QString>

DoserWidget::DoserWidget(QWidget *parent) : QWidget(parent)
{
	QVector<QGroupBox*> groups = createGuiGroups();

	mainLayout = new QGridLayout;
	mainLayout->addWidget(groups[0], 0, 0);
	mainLayout->addWidget(groups[1], 0, 1);
	mainLayout->addWidget(groups[2], 0, 2);
	mainLayout->addWidget(groups[3], 0, 3);

	mainLayout->addWidget(new QPushButton("Segmentation"), 1, 0);
	mainLayout->addWidget(new QPushButton("Open"), 1, 1);
	mainLayout->addWidget(new QPushButton("Save"), 1, 2);
	mainLayout->addWidget(new QPushButton("Save"), 1, 3);

	setLayout(mainLayout);
	changeGuiMode();
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

void DoserWidget::displayGridColumn(int column, bool isVisible)
{
	for (int i = 0; i < mainLayout->rowCount(); ++i)
	{
		mainLayout->itemAtPosition(i, column)->widget()->setVisible(isVisible);
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
