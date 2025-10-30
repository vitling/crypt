/*
    Copyright 2025 David Whiting

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once
#include <JuceHeader.h>

/** Simple encapsulation of the FIFO buffer to communicate data from the audio thread to the GUI thread.
 *  I don't really actually understand it fully so I thought best to get it all inside here instead of
 *  spread around the rest of the code
 */
class SharedBuffer {
    private:
        AbstractFifo fifo;
        std::vector<float> writeBuffer;
        std::vector<float> readBuffer;
        int size;
    public:
        SharedBuffer(int size): fifo(size), writeBuffer(size, 0.0f), readBuffer(size, 0.0f) {

        }

        /** Call this from the audio thread only */
        void write(int count, const float * readPointer) {
            int start1, size1, start2, size2;
            fifo.prepareToWrite(count, start1, size1, start2, size2);

            if (size1 > 0)
                std::copy(readPointer, readPointer + size1, writeBuffer.begin() + start1);
            if (size2 > 0)
                std::copy(readPointer + size1, readPointer + size1 + size2, writeBuffer.begin() + start2);

            fifo.finishedWrite(size1 + size2);
        }

        /** Call this from the GUI thread only */
        const std::vector<float> & read() {
            int start1, size1, start2, size2;
            fifo.prepareToRead(512, start1, size1, start2, size2);

            if (size1 > 0)
                std::copy(writeBuffer.begin() + start1,
                        writeBuffer.begin() + start1 + size1,
                        readBuffer.begin());
            if (size2 > 0)
                std::copy(writeBuffer.begin() + start2,
                        writeBuffer.begin() + start2 + size2,
                        readBuffer.begin() + size1);

            fifo.finishedRead(size1 + size2);

            return readBuffer;
        }

        /** Call this from the GUI thread only */
        const std::vector<float> & get() const {
            return readBuffer;
        }

};

