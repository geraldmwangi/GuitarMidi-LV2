import tensorflow as tf
from tensorflow.keras import layers, models
def build_cnn_model(input_shape, output_dim,training=True):
    model = models.Sequential()
    model.add(layers.Input(shape=input_shape, dtype=tf.float32))

    model.add(layers.SeparableConv2D(filters=32, kernel_size=(3, 3), padding='same', activation='relu'))
    model.add(layers.BatchNormalization())
    if training:
        model.add(layers.SpatialDropout2D(0.2))
    model.add(layers.MaxPooling2D(pool_size=(2, 2), strides=(2, 2)))

    model.add(layers.SeparableConv2D(filters=64, kernel_size=(3, 3), padding='same', activation='relu'))
    model.add(layers.BatchNormalization())
    if training:
        model.add(layers.SpatialDropout2D(0.2))
    model.add(layers.MaxPooling2D(pool_size=(2, 2), strides=(2, 2)))

    model.add(layers.SeparableConv2D(filters=128, kernel_size=(3, 3), padding='same', activation='relu'))
    if training:
        model.add(layers.BatchNormalization())
    model.add(layers.SpatialDropout2D(0.3))
    model.add(layers.MaxPooling2D(pool_size=(2, 2), strides=(2, 2)))

    model.add(layers.GlobalAveragePooling2D())
    if training:
        model.add(layers.Dropout(0.2))
    model.add(layers.Dense(output_dim, activation='sigmoid', dtype=tf.float32))

    return model