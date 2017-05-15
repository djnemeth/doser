#ifndef DOSERWIDGET_H
#define DOSERWIDGET_H

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QImage>
#include <QLabel>
#include <QMap>
#include <QPoint>
#include <QProgressBar>
#include <QPushButton>
#include <QThread>
#include <QVector>
#include <QWidget>

#include "dosermodel.h"

class DoserWidget : public QWidget
{
	Q_OBJECT

public:
	enum GuiElementType
	{
		SOURCE, DEEP, QUICK
	};

	static const int DEEP_GROUP_COLUMN_INDEX = 2;
	static const int QUICK_GROUP_COLUMN_INDEX = 3;

	explicit DoserWidget(QWidget* parent = nullptr);
	~DoserWidget();

signals:
	void doSegment(DoserModel::SegmentationMode mode);
	void doOpenImage(const QString& path);
	void status(const QString& message);

private slots:
	void changeGuiMode();
	void drawSegment(DoserModel::SegmentationMode mode, const DoserModel::Segment &segment);
	void imageChanged(const QImage& image);
	void subProcessProgressChanged(DoserModel::SubProcessType type, int current, int max);
	void openImage();
	void segment();
	void SegmentationStarted(DoserModel::SegmentationMode mode);
	void segmentationFinished(DoserModel::SegmentationMode mode, const QVector<DoserModel::Segment>& finalSegments);
	void segmentationProgressChanged(int current, int max);

private:
	DoserModel::SegmentationMode currentMode() const;
	void setButtonsEnabled(bool enabled);
	void setupModel();
	void setupGui();
	QGroupBox* createSettingsGui();
	QVector<QGroupBox*> createGuiGroups();
	void displayGridColumn(int column, bool isVisible);
	GuiElementType toGuiElementType(DoserModel::SegmentationMode mode) const;
	QString toString(DoserModel::SegmentationMode mode) const;
	QString toString(DoserModel::SubProcessType type) const;
	void resetImages();

	DoserModel* model;
	QThread modelThread;
	QMap<GuiElementType, QImage> images;

	QGridLayout* gridLayout;
	QComboBox* modeComboBox;
	QMap<GuiElementType, QLabel*> imageLabels;

	QPushButton* segmentButton;
	QPushButton* openButton;
	QMap<GuiElementType, QPushButton*> saveButtons;

	QProgressBar* mainProgressBar;
	QProgressBar* subProgressBar;
};

#endif // DOSERWIDGET_H
