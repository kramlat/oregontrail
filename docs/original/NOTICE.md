# The reference listing

`oregon-1978-creative-computing.bas` is a BASIC listing of The Oregon Trail as published in
*Creative Computing* in 1978 (sources disagree on the exact issue: one says May/June, the
file's own header says July/August). The program is by Don Rawitsch, Bill Heinemann and
Paul Dillenberger, with MECC (the Minnesota Educational Computing Consortium).

This copy comes from
[fortran-gaming/oregon-trail-1975](https://github.com/fortran-gaming/oregon-trail-1975)
(`oregon-trail-1978.bas`), which lightly modified it to run on modern BASIC interpreters
and publishes it under the MIT licence reproduced below. We include it only as a reference
to check `src/oregon.cpp` against; the game does not run it. A second transcription,
[topherPedersen/OregonTrail1978](https://github.com/topherPedersen/OregonTrail1978)
(Unlicense), was used to cross-check the text. They differ at line 2600, where that copy has
`100+RND(1)` and this one has `100*RND(-1)`; we follow the multiplication.

The rest of this repository is GPL-3.0-or-later (see `LICENSE`). That licence does not
cover this listing, which stays under the terms below and the rights of its authors.

---

MIT License

Copyright (c) 2021 Michael Hirsch, Ph.D.,

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


Copyright (c) 2021, Philipp Engel

Permission to use, copy, modify, and/or distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright notice
and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.
