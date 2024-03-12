.. _roq-pubsub:

roq-pubsub
==========


Purpose
-------

* Support publish / subscribe of custom data types.


Description
-----------

The "gateway" only supports :code:`CustomMetrics` and :code:`CustomMatrix`.


Conda
-----

* :ref:`Using Conda <tutorial-conda>`

.. tab:: Install
  
  .. code-block:: bash
  
    $ mamba install \
      --channel https://roq-trading.com/conda/stable \
      roq-pubsub
  
.. tab:: Configure

  .. code-block:: bash
  
    $ cp $CONDA_PREFIX/share/roq-pubsub/config.toml $CONFIG_FILE_PATH
  
    # Then modify $CONFIG_FILE_PATH to match your specific configuration
  
.. tab:: Run
  
  .. code-block:: bash
  
    $ roq-pubsub \
          --name "pubsub" \
          --config_file "$CONFIG_FILE_PATH" \
          --service_listen_address "$TCP_LISTEN_PORT_FOR_METRICS" \
          --flagfile "$FLAG_FILE"
  

Config
------


Flags
-----

* :ref:`Using Flags <abseil-cpp>`

.. code-block:: bash

   $ roq-udp-subscriber --help

.. tab:: Flags

   .. include:: flags/flags.rstinc


Constraints
-----------
