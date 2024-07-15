#include "AudioPreview.h"

#include <cmath>

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QProgressBar>
#include <QStyle>
#include <QTime>
#include <QTimer>
#include <QToolButton>

#include "../FileViewer.h"

SeekBar::SeekBar(QWidget* parent)
		: QProgressBar(parent) {
	this->setRange(0, 1000);
	this->setOrientation(Qt::Horizontal);
	this->setTextVisible(false);

	auto* layout = new QHBoxLayout(this);
	layout->setContentsMargins(0,0,0,0);

	this->label = new QLabel(this);
	this->label->setAlignment(Qt::AlignCenter);
	QObject::connect(this, &QProgressBar::valueChanged, this, [this](int value) {
		QString formatter;
		if (AudioPlayer::getLengthInSeconds() >= 60 * 60 * 24) {
			formatter = "dd:hh:mm:ss.zzz";
		} else if (AudioPlayer::getLengthInSeconds() >= 60 * 60) {
			formatter = "hh:mm:ss.zzz";
		} else if (AudioPlayer::getLengthInSeconds() >= 60) {
			formatter = "mm:ss.zzz";
		} else {
			formatter = "ss.zzz";
		}
		auto text = QString("%1 / %2").arg(
				QTime(0,0).addMSecs(static_cast<int>(AudioPlayer::getPositionInSeconds() * 1000)).toString(formatter),
				QTime(0,0).addMSecs(static_cast<int>(AudioPlayer::getLengthInSeconds() * 1000)).toString(formatter));
		this->label->setText(text);
	});
	layout->addWidget(this->label);
}

void SeekBar::mousePressEvent(QMouseEvent* event) {
	this->processMouseEvent(event);
	QWidget::mousePressEvent(event);
}

void SeekBar::processMouseEvent(QMouseEvent* event) {
	if (event->button() != Qt::MouseButton::LeftButton) {
		return;
	}
	event->accept();
	auto percent = static_cast<double>(event->pos().x()) / static_cast<double>(this->width());
	emit this->seek(static_cast<std::int64_t>(percent * static_cast<double>(AudioPlayer::getLengthInFrames())));
}

AudioPreview::AudioPreview(FileViewer* fileViewer_, QWidget* parent)
		: QWidget(parent)
		, fileViewer(fileViewer_)
		, playing(false) {
	this->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	this->setUpdatesEnabled(true);

	auto* layout = new QGridLayout(this);
	// todo: qt stretch 20 hack
	layout->setColumnStretch(0, 20);
	layout->setColumnStretch(2, 20);
	layout->setRowStretch(0, 20);
	layout->setRowStretch(3, 20);
	layout->setSpacing(0);

	auto* controls = new QWidget(this);
	layout->addWidget(controls, 1, 1);

	auto* controlsLayout = new QHBoxLayout(controls);
	controlsLayout->setSpacing(0);

	this->playPauseButton = new QToolButton(controls);
	this->playPauseButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
	this->playPauseButton->setShortcut(Qt::Key_Space);
	QObject::connect(this->playPauseButton, &QToolButton::pressed, this, [this] {
		if (!this->playing && AudioPlayer::getPositionInFrames() == AudioPlayer::getLengthInFrames()) {
			AudioPlayer::seekToFrame(0);
		}
		this->setPlaying(!this->playing);
	});
	controlsLayout->addWidget(this->playPauseButton);

	controlsLayout->addSpacing(4);

	this->seekBar = new SeekBar(controls);
	this->seekBar->setFixedWidth(300);
	QObject::connect(this->seekBar, &SeekBar::seek, this, [this](std::int64_t frame) {
		AudioPlayer::seekToFrame(frame);
		if (!this->playing) {
			this->setPlaying(true);
		}
	});
	controlsLayout->addWidget(this->seekBar);

	this->infoLabel = new QLabel(this);
	layout->addWidget(this->infoLabel, 2, 1, Qt::AlignHCenter);

	this->setPlaying(false);

	auto* timer = new QTimer(this);
	QObject::connect(timer, &QTimer::timeout, this, [this] {
		if (AudioPlayer::getLengthInFrames() > 0) {
			this->seekBar->setValue(static_cast<int>(std::round((static_cast<double>(AudioPlayer::getPositionInFrames()) / static_cast<double>(AudioPlayer::getLengthInFrames())) * this->seekBar->maximum())));
			if (AudioPlayer::getPositionInFrames() == AudioPlayer::getLengthInFrames()) {
				this->setPlaying(false);
			}
			this->infoLabel->setText(tr("Sample Rate: %1hz\nChannels: %2").arg(AudioPlayer::getSampleRate()).arg(AudioPlayer::getChannelCount()));
		} else {
			this->seekBar->setValue(0);
		}
	});
	timer->start(5);
}

AudioPreview::~AudioPreview() {
	AudioPlayer::deinitAudio();
}

void AudioPreview::setData(const std::vector<std::byte>& data) {
	this->persistentAudioData = data;
	if (auto err = AudioPlayer::initAudio(this->persistentAudioData.data(), this->persistentAudioData.size()); !err.isEmpty()) {
		this->fileViewer->showInfoPreview({":/icons/warning.png"}, err);
	} else {
		this->setPlaying(true);
	}
}

void AudioPreview::paintEvent(QPaintEvent* event) {
	QWidget::paintEvent(event);
}

void AudioPreview::setPlaying(bool play) {
	this->playing = play;
	if (this->playing) {
		AudioPlayer::play();
		this->playPauseButton->setIcon(this->style()->standardIcon(QStyle::SP_MediaPause));
	} else {
		AudioPlayer::pause();
		this->playPauseButton->setIcon(this->style()->standardIcon(QStyle::SP_MediaPlay));
	}
}
