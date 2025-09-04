.. _roq-pubsub:

roq-pubsub
==========

Support publish / subscribe of custom data types.


Installing
----------

* :ref:`Using Conda <tutorial-conda>`

.. tab:: Unstable

  .. code-block:: shell

     $ conda install \
           --channel https://roq-trading.com/conda/unstable \
           roq-pubsub

.. tab:: Stable

  .. code-block:: shell

     $ conda install \
           --channel https://roq-trading.com/conda/stable \
           roq-pubsub


Using
-----

.. code-block:: shell

   $ roq-pubsub \
         --name "pubsub" \
         --config_file $CONFIG_FILE_PATH \
         --client_listen_address $UNIX_SOCKET_PATH \
         --flagfile $ENVIRONMENT_FLAGFILE


.. _roq-pubsub-flags:

Flags
-----

* :ref:`Using Flags <abseil-cpp>`
* :ref:`Gateway Flags <gateway-flags>`

.. code-block:: shell

   $ roq-pubsub --help

.. tab:: Flags

   .. include:: flags/flags.rstinc


Constraints
-----------

* The "gateway" only supports :code:`CustomMetrics` and :code:`CustomMatrix`.
