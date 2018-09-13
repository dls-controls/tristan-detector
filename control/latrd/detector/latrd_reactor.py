import zmq
import datetime

import logging

from latrd_message import LATRDMessage, GetMessage, PutMessage, PostMessage, ResponseMessage


class LATRDReactorTimer:

    last_timer_id = 0

    def __init__(self, delay_ms, times, callback):
        self._log = logging.getLogger(".".join([__name__, self.__class__.__name__]))
        LATRDReactorTimer.last_timer_id += 1
        self._timer_id = LATRDReactorTimer.last_timer_id
        self._delay_ms = delay_ms
        self._times = times
        self._callback = callback
        self._when = self.clock_mono_ms() + delay_ms
        self._expired = False

    def get_id(self):
        return self._timer_id

    def do_callback(self):
        self._callback()

        if (self._times > 0) and ((self._times - 1) == 0):
            self._expired = True
        else:
            self._when += self._delay_ms

    def has_fired(self):
        return LATRDReactorTimer.clock_mono_ms() >= self._when

    def has_expired(self):
        return self._expired

    def when(self):
        return self._when

    @staticmethod
    def clock_mono_ms():
        time = datetime.datetime.now()
        time_ms = int(time.strftime("%s")) * 1000.0 + int(time.microsecond / 1000.0)
        #log.debug("%12d", time_ms)
        return time_ms


class LATRDReactor:

    def __init__(self):
        logging.basicConfig(format='%(asctime)-15s %(message)s')
        self._log = logging.getLogger(".".join([__name__, self.__class__.__name__]))
        self._log.setLevel(logging.DEBUG)
        self._terminate_reactor = False
        self._pollitems = {}
        self._callbacks = {}
        self._timers = {}
        self._pollsize = 0
        self._needs_rebuild = True
        self._channels = {}

    def register_channel(self, channel, callback):
        # Add channel to channel map
        self._channels[channel.socket] = channel
        self._callbacks[channel.socket] = callback
        # Signal a rebuild is required
        self._needs_rebuild = True

    def remove_channel(self, channel):
        # Remove the channel from the map
        self._channels.pop(channel.socket)
        # Signal a rebuild is required
        self._needs_rebuild = True

    def register_timer(self, delay_ms, times, callback):
        timer = LATRDReactorTimer(delay_ms, times, callback)
        self._timers[timer.get_id()] = timer
        return timer.get_id()

    def run(self):
        rc = 0;

        # Loop until the terminate flag is set
        while not self._terminate_reactor:
            # If the poll items list needs rebuilding, do it now
            if self._needs_rebuild:
                self.rebuild_pollitems()

            # If there are no channels to poll and no timers currently active, break out of the
            # reactor loop cleanly
            if self._pollsize == 0:
                rc = 0
                break

            try:
                # Poll the registered channels, using the tickless timeout based
                # on the next pending timer
                pollrc = dict(self._poller.poll(self.calculate_timeout()))

                for sock in pollrc:
                    if pollrc[sock] == zmq.POLLIN:
                        try:
#                            reply = self._channels[sock].recv()
#                            msg = LATRDMessage.parse_json(reply)
                            self._callbacks[sock]()
                        except Exception as e:
                            # TODO: How to handle an exception here
                            self._log.debug("Caught reactor exception")
                            #self._log.exception(e)

                for timer in self._timers:
                    if self._timers[timer].has_fired():
                        self._timers[timer].do_callback()
                    if self._timers[timer].has_expired():
                        self._timers.pop(timer)

            except Exception as e:
                self._log.exception(e)
                break

        return rc

    def rebuild_pollitems(self):
        # If the existing pollitems array is valid, delete it
        self._poller = zmq.Poller()

        self._pollsize = len(self._channels)

        if self._pollsize > 0:
            for channel in self._channels:
                self._log.debug("Registering %s for polling", channel)
                self._poller.register(channel, zmq.POLLIN)

        self._needs_rebuild = False

    def calculate_timeout(self):
        # Calculate shortest timeout up to one hour (!!), looping through
        # current timers to see which fires first
        tickless = LATRDReactorTimer.clock_mono_ms() + (1000 * 3600)
        for timer in self._timers:
            if tickless > self._timers[timer].when():
                tickless = self._timers[timer].when()

        # Calculate current timeout based on that, set to zero (don't wait) if
        # there is no timers pending
        timeout = tickless - LATRDReactorTimer.clock_mono_ms()
        if timeout < 0:
            timeout = 0
        return timeout
