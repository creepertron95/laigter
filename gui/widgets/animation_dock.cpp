#include "animation_dock.h"
#include "ui_animation_dock.h"

#include "src/image_loader.h"

#include <QDebug>
#include <QFileDialog>
#include <QThread>
#include <QMessageBox>

AnimationDock::AnimationDock(QWidget *parent) : QWidget(parent),
                                                ui(new Ui::AnimationDock)
{
  ui->setupUi(this);
  m_current_processor = nullptr;
}

AnimationDock::~AnimationDock()
{
  delete ui;
}

void AnimationDock::setCurrentProcessor(ImageProcessor *p)
{
  this->disconnect();

  disconnect(m_current_processor, SIGNAL(frameChanged(int)), this, SLOT(setCurrentFrame(int)));

  m_current_processor = p;

  ui->playButton->setChecked(p->animation.isActive());
  for (int i = 0; i < ui->listWidget->count(); i++)
  {
    delete ui->listWidget->item(i);
  }
  ui->listWidget->clear();
  for (int i = 0; i < p->frames.count(); i++)
  {
    QListWidgetItem *item = new QListWidgetItem;
    QImage image;
    p->frames[i].get_image(TextureTypes::Diffuse, &image);
    item->setIcon(QPixmap::fromImage(image));
    ui->listWidget->addItem(item);
  }

  ui->listWidget->setCurrentRow(p->get_current_frame_id());
  ui->fpsSpinBox->setValue(1 / (p->animation.interval() / 1000.0));

  connect(m_current_processor, SIGNAL(frameChanged(int)), this, SLOT(setCurrentFrame(int)));
  connect(ui->listWidget, SIGNAL(currentRowChanged(int)), this, SLOT(updateProcessorFrame(int)));
}

void AnimationDock::updateProcessorFrame(int index)
{
  if (index >= 0 && index < m_current_processor->frames.count() && ui->listWidget->selectedItems().count() > 0)
  {
    if (!m_current_processor->animation.isActive())
    {
      m_current_processor->set_current_frame_id(index);
    }
  }
}

void AnimationDock::on_fpsSpinBox_valueChanged(int arg1)
{
  m_current_processor->setAnimationRate(arg1);
}

void AnimationDock::on_playButton_toggled(bool checked)
{
  m_current_processor->playAnimation(checked);
  checked ? ui->playButton->setIcon(QIcon::fromTheme("pause")) : ui->playButton->setIcon(QIcon::fromTheme("play"));
}

void AnimationDock::on_leftButton_clicked()
{
  QList<QListWidgetItem *> item_list = ui->listWidget->selectedItems();
  if (item_list.count() > 0)
  {
    int index = ui->listWidget->currentRow();
    if (index == 0)
    {
      return;
    }
    m_current_processor->playAnimation(false);
    Sprite s(m_current_processor->frames[index]);
    m_current_processor->frames[index] = m_current_processor->frames[index - 1];
    m_current_processor->frames[index - 1] = s;
    ui->listWidget->insertItem(index - 1, ui->listWidget->takeItem(index));
    ui->listWidget->setCurrentRow(index - 1);
  }
}

void AnimationDock::setCurrentFrame(int index)
{
  ui->listWidget->setCurrentRow(index);
}

void AnimationDock::on_rightButton_clicked()
{
  QList<QListWidgetItem *> item_list = ui->listWidget->selectedItems();
  if (item_list.count() > 0)
  {
    int index = ui->listWidget->currentRow();
    if (index >= m_current_processor->frames.count() - 1)
    {
      return;
    }
    m_current_processor->playAnimation(false);
    Sprite s(m_current_processor->frames[index]);
    m_current_processor->frames[index] = m_current_processor->frames[index + 1];
    m_current_processor->frames[index + 1] = s;
    ui->listWidget->insertItem(index + 1, ui->listWidget->takeItem(index));
    ui->listWidget->setCurrentRow(index + 1);
  }
}

void AnimationDock::on_deleteFrameButton_pressed()
{
    bool isPlaying = m_current_processor->animation.isActive();
    m_current_processor->animation.stop();
    m_current_processor->remove_frame(ui->listWidget->currentRow());
    delete ui->listWidget->takeItem(ui->listWidget->currentRow());
    if (m_current_processor->frames.count() <= 1){
        parentWidget()->hide();
    }else{
        if (isPlaying) m_current_processor->animation.start();
    }

}

void AnimationDock::on_addFrameButton_pressed()
{
    bool isPlaying = m_current_processor->animation.isActive();
    m_current_processor->animation.stop();
    QStringList fileNames = QFileDialog::getOpenFileNames(
        this, tr("Open Image"), "",
        tr("Image File (*.png *.jpg *.bmp *.tga)"));
    QSize frameSize = m_current_processor->get_current_frame()->size();
    foreach (QString fileName, fileNames)
    {
      if (fileName != nullptr)
      {
        bool success;
        QImage image = ImageLoader::loadImage(fileName, &success);
        if (!success)
          continue;


//        fs_watcher.addPath(fileName);
        image = image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);

        if (image.size() != frameSize){
            QMessageBox msgBox;
            msgBox.setText(tr("Cannot open ") + fileName + ".\n" +
                           tr("New frame must have the same size than original frames."));
            msgBox.exec();
            continue;
        }

        m_current_processor->loadImage(fileName, image);
      }
    }
    if (isPlaying) m_current_processor->animation.start();
    setCurrentProcessor(m_current_processor);
}

void AnimationDock::on_deleteEmptyButton_pressed()
{
    bool isPlaying = m_current_processor->animation.isActive();
    m_current_processor->animation.stop();

    QImage empty = m_current_processor->get_texture()->copy();
    QImage test;
    empty.fill(Qt::transparent);

    for (int i=0; i<m_current_processor->frames.count(); i++ ) {
        m_current_processor->frames[i].get_image(TextureTypes::Diffuse,&test);
        if (empty == test){
            m_current_processor->remove_frame(i);
            i--;
        }
    }

    if (isPlaying) m_current_processor->animation.start();
    setCurrentProcessor(m_current_processor);

}
