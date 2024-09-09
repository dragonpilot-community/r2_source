#include "selfdrive/ui/qt/sidebar.h"

#include <QMouseEvent>

#include "selfdrive/ui/qt/util.h"

// dp - for satellite fmin
#include <cmath>

void Sidebar::drawMetric(QPainter &p, const QPair<QString, QString> &label, QColor c, int y) {
  const QRect rect = {30, y, 240, 126};

  p.setPen(Qt::NoPen);
  p.setBrush(QBrush(c));
  p.setClipRect(rect.x() + 4, rect.y(), 18, rect.height(), Qt::ClipOperation::ReplaceClip);
//  p.drawRoundedRect(QRect(rect.x() + 4, rect.y() + 4, 100, 118), 18, 18);
  p.drawRect(QRect(rect.x() + 4, rect.y() + 4, 100, 118));
  p.setClipping(false);

  QPen pen = QPen(QColor(0xff, 0xff, 0xff, 0x55));
  pen.setWidth(2);
  p.setPen(pen);
  p.setBrush(Qt::NoBrush);
//  p.drawRoundedRect(rect, 20, 20);
  p.drawRect(rect);

  p.setPen(QColor(0xff, 0xff, 0xff));
  p.setFont(InterFont(35, QFont::DemiBold));
  p.drawText(rect.adjusted(22, 0, 0, 0), Qt::AlignCenter, label.first + "\n" + label.second);
}

Sidebar::Sidebar(QWidget *parent) : QFrame(parent), onroad(false), flag_pressed(false), settings_pressed(false) {
  home_img = loadBased64Image("iVBORw0KGgoAAAANSUhEUgAAAQoAAAEKCAMAAADdFev7AAAAVFBMVEVHcEz///////////////////////////////////////////////////////////////////////////////////////////////////////////+DS+nTAAAAG3RSTlMAKEIf6zXh+gTzUBYOCM3XwoaRcHq4nVymZbCV8JeNAAAgAElEQVR42uxc2WKrOgysAe/GNmaH///PC17AEHoKSdqk5x6eWkIKlqXRaCT68fHv+Hf8O/4dv+cg/0zgD9g1Rdmm/69F86Qvmq6p2GbdbJwPs/gGgTDl+V/tKqCQ1K56xKqIjNHbc1myGExmSGr29xoi79EYHXJZ+UflznR5MFk2/9r/tZYQBXYOQb1nZMu2G3eChhNwvgIvHxPG/i4cqawlZMVg0mu7dAT9R513lEF47JgvpXCNF6wM43+NJRLr9I3bXm5wFBF57U2BW3dta50GLF+1TqJM8iutQYTYJoB8mJdXh8UI+2vmNl7IgB/afV7OP6t0Fz8jlkUifpcd0tYMWndlHOAw24S/2+mxctcrGzrTGeygsrC/h1VztWIt1eXvcY08aZDDx1H1+XK63W70h7Bw0ZE1YZSzo0h7RWM9KHyZOZBR/q9K9ksIB+zouoe4yDf5Ukfu3aw77xOGdZTZT8iwminERwbTtnPJmFb5b7AEQz5heluU4YNibwprG2SdILEJIyFzIlEgeIwJ4aaCjxDQa7z5q+9sCevqyrRJW8c46FeuojhvV9wMCcPiSRHAodjEh1+9sCk5g+8fHbNPYJO6fDljHCAxt17zY8iQbEkYk4MQ48hGimKy2Yzx4vMu9pj3zaD2qY2PZF5FmY/4IIjqkZVcu4TBvSmN846RxfEx5Btvkt69CHhT2LBLUOnnnwUKtS6xXPZd594oGbQOE8zW4m09YuMF+ZtA1bxnrFhH73aVByTRyquoXF9M4RLGMF8H5pNNG+EB2caHdyFvivnDzIA3NEUTo53z31L7QsNlhSYKEBTcJP5oRtdsiDbe4QbWJXShYI3lXCjwtPr9GCjZIRpnDVo9odlxLOvoFkfjhJHOHBxH2ab1xGrM6hJynrhCrorI/PiOqbWLSCIBlXSrkPGalsd2RYhFQ7fx/pM+LN1xENJFCkemlOMryMUEo5t8/VZleBTWfbZQTrbu96JU5Va8cALFBlG59l+ryRJGo6yzWPShZRRzMRS/EcHCq6fPP2NEIwbt9htVQIiUWTjwaqZjGCHPhohoIqmvzWGlF0KflfEffAekEDdZk8s1ILjMhjatI3fOfa2NpHQmGju+iryLUCO8eGEt6pKLRVABq0FlWSYNjO9GIwU0eVEuAV1ceEaJzgdEMkO+2zgPnMLQjZsbH+RtRnFERB0CuC+5+AiFWc4BBHyTuiPuNRE0lbwiW7AJ9nEHD3jUWlIGNhGALW8lXrx8SMJlAkDW9oufO2rtSFX/ORq4P72q5BZlsvLH2ScvPJSXm0glw44P+RTK1/xqtFJKN4EmHBJT09XSbrDLlsf8tXCIspg9t7fC5qdtAYOr42HjGA7zmuVxoNrV1GTycs6/elySC3uN41PNYXyindVLe2tc/bhXLGrklBL4/jz22opo1dYy1xWxQlJ82Boy45baOhX5gXs9xLGxuzuuI7ByOImbhHOvs4Q8ca/Rk4p/Vt2F3DQ7idsb/YJ+SW83v3MomK1P60jklB6UyrZ54hs4/lrd+duiV5SpTnDKS9/604tjpHWcMLH+Hk3W1WEy+ABxGEpf0lq0rYupTIB+5VkRNj81C0mmdfs99YHLLHhZeetQvHiNFG48fvPKr3zZf5IYjbJM1d/Xv3EMf6nDoPPN4UV1WbtUk75unlAhXTk5AOn3YTnf1mGpewD1ItpNrDRplSdHHrbNmu/11G0d5rhV1Jf/cbDQIZnBGq9lxU8kM4cUSx3mudULBRzjfNTPj4TeoPyJdAYKtM6lsJdxqy10mbSxEowBziLfwyIOcnkTilDHzV/CrbZFALIkD7X5h02r6k8sQoinYpW7E38ht1ojduFSviITVfNHDAeGPT25Bm71k6Ie4TfZsfDkqhIf5/JGn+k/1eZ35RLqGvY/x61I0qgM1W1+y3MkO50588l4mS4YEE978op+LmhMfspT/mQ0FYWvQbv4nlazp1cktNw+Oc5kV/QMAi4efk5i0Zo26dH2FVohVVfPhBG+9iI6sQeLSxUQYYvOgWmmZD3PMyfwkb2Dlmrqm/WCJpRCWfG07CKirsxGZqx2QuYps5Z6o/VaL0GzTfoE8DsCh7h0jtrtdxMZ3UE/yTEcSNPMrcDsK2V11eQ8mUV8PN4cNJOD6eHlLJPbgpAWscOWmxbSqJ5jixmk8ZAAVu/75FZzxndQf8IhK2ctF92aJJPF5SzD3AjGksp5g8MkudZoo2w8JJLM9m1EYDPmgyxgZ/Xt+/t0JOfpZJLGTcBnih70Bc7SPbtNgYAu6pFu01wAK853j2cSqxn6wdMpIHDZD7Ku0gUsUP/oPQhPzLSrdCjN0iWhzcWCm9tuk+sgcm8JGtQk69cPkzCrGQYhYKrLsQpkwtmmfk4QgjlbqxKwdeTzYty5rGq7Ip76RUBaPKHfnhdxA6pdAzuzrSDe87sCQ3CepsAfqc0bNidqlqdt5yKFlheDZMoZtiZzWvgma8yghtnjkLn0GpZ2v2sFMbCxAxFfAQOASdtXphlqLZVCKHMHksOs/PFCImldOk2qZrrkssOlrq/s3i3ZkEErKDw21JegSM12STWT4fUWjNg2pR1XpUSkSVuZYWJ+lI6fHHZye6pyuBBuTH7ynOvsnKzEr966a7Ubn70OFB5/UD/fZFaVcQM8IkdDiYvdGrh7egFY1YUJgi8O2RJYSym1dhQUpPdVKnaaaT/iWuzGZ+9QZ1TAYuE8xCZV4oaXd4b/IBXOmjUL5oAZfcSkPvUMwxmKaPkUOKZk8KpabMnOjgJb8fExU/h8PdOXJEqqH3DAB/ryDOK0S9IJE5PeaHTBDH7kKE3UnpRTVZtLb11ar9j1kG2bRD4olvi3eaY929BXwYqjinQugaiS6lxI+EmzyGI6hfLgGqzK88uwWLE1hVPku0eVgYjNn1DXSTJc9AWsTVSfdTmoD6+6MDVR7JftXe0JTcSlxjtFUuLBmnOHLFmxgEr/wbuj7+PzC5mLxEjqzP2ooHpGFQKGQCVOsWHOqqYb6pk8IHrGLLhuIZy+NH1nYgY51EcXnU+Fdn5pELsIx8/pLPMwTiZPS1Ykn4jBRKraQp4xBuqqNkkSNhVn+hho6KeE68ZGM93EjS+bQrw9ay6JhHnUrL+IPbk43uQje1D6JydqyCda4+1227dndD+5WlCycCRmkCcBBj7d8uGwnXxeS5mNTzlotdn+qcCfX+CHw3g7fOWbqHRhNWjZQZGY6lmAMZ4CjOmOko7PPbAuyqLqEyAE6E2tpqMessNhd96Nxy+/i4behZ+E84j7isC3my89TLQ1kl1RDPc6BNWmnCq3Th3Jfkjqzd/tD8Nm3YhYprcvl1wGUFANUsluffnL94m/lglzVrRgopwAbv8zwQXa6fl72tY3xrg50X9CcKh/YXmjLc2Ztr6GoLwIq0DNMjszAwY9VfaDfqrCpvr7XrdAne8pihtrqqqmX5tiHgkvm64rdi/1z9XIpdbNx4bzUR26YmA4MyFKkgY9jpJhNDbZmZMmeTJsGMfn4EVuY7m8qFyAXQLEHQj06WumAyZomtA7ww8aA1Ugzzm7YfJmES/Hs+C1a+mh88DphyAxPmX6PVDUuilZAmH/cBZFuj7IQwp2u4ruktxlLr1hZSd6VMFYv3r6cJL48inZ+R+b8VsOfFPkXGIKM3CeZvF2jsVnoDT09e7g8N9kiltYuaDrE6vBnQbOalOFCuYS2kVpkPAW/ZApzg9XCObdvDkvA23MJip6aXognzl3J/EPWQKdhTGwdq5PfmfudmwVH6d1n4oQMdWitcqebwY6H+MR3zqnTolthj8FnLMgtm+fWL2wOEEozB3FFz4sSOP0heuphIcQJm38b0LGrOmZObeqdD/KcCrcDxHWnDFFai5XYFT+x9x1Lciq48AhGIzJYOL5//9c2wQHHOme2duPu+f2NEKWSiWpPGMUx/E2K/zutMTjhSqFtmx94emLX0R5ziXW4mVDGuWCOh4ykSgdOs9e2aBbZsEeKYsSE5SfHseh9Sy6MKB/iFR9xI5Y9F8Wtau4n5pmwpEc+S4LMcy43JuI1ciiU7ov8ntMNhGoF9OWaEKAEY1R0SW5u9O7Y0XcFusYH12LwcMQbX8MlSRDm7HVUZHj6W/mp0xAHINSNgaXwlguDyZ1OaniaTGcix3GSHot2RDX53lP3YFzVAsiFgR2G6N2do/4UESceqDqE/ye0010nURYqrknQwCJPnmeNpICFOv6HIu9y+ms5GsK2iGAdOZZ4NpQLjjhlghv1onLqk6gSsbLcnRwtymtAXNF9yEtnRSesJ96d+fJuxRscdQVFb6N2sWKKY6aajuNhrknRX1b3MJL97qfUGAf7GfjouPqUcQDxKGqy1V6e6rim6TL5GwD8b1YwfTZdu3C3WP9o/BbhFdND/EZ95YDaYJMfLRk25EY5ZjmjvTmdq/ZkxKtqRJm/Iv8CmA3tBJ5c8GB/xVDBTLxiMAECbwgf9UJd9Kzo9oroYyvy0cpcYhHPU3+bN77IAvQi0xa249+1A9VE/CAFVKRKwfYKUL3F5ySrMS43TVjdhL6TDBoVHCPibGEGEfPZFFOq69QY7KIGSj3cadqmbxghbStUClhpUVjIbclIfXTi9bbk8sUUqeOqivN1qGLpy0COA4Zl7g78vHkB7ab6JG6Jawxd/+efcnq3uFk4RYUWay0SfPf3ZYS0arrYPmWocreRvQ8UIW24ozOTiqbRwSN/OBV484Hn37uotZBFMHDrbO5Hzqrc6isR2SLLRKjXC/N3d0po0cQ/IOl/PoQ73HAVGqJvGHymr3VKdRB0sTqTPI0S4Jb/UoWwTHt36xLlcue2U/iUtB2A/uFWxFwPJ5hU/koLGJ0dCSU3e5q/kM5qDqylrQEZ5+GgA5q98m6253oMWESsSXvYo35/04H//f/iEZY1FwVwrH5YkGZQEsJ8Abg1CnjfDmGcgKE4DAGlQurYV1FY6tmpf/fBy7XSO89b2+MFM+Ic+rapBOOo7Ku6yoByyAAtYLbIhkOzTBwZPm0nfadMUXNf0ZQsNZ11LTuroNpBGoX6nJdJYhg8cmI/u7kJJugGBY8Gf/7n9g5XqNvSqFetwxTxzf4PztzIGVlUBUdGH+bqRpC1g3xrywTwg+GdDwwVmAHOzkl4P+1tGVLO3YL40XO4wCZGsJH8RLawPOwgFewDY4e9Ucf+q2XNAalN6g4Qc+EXdavTFXVSYxXYKU10+am5QK+l6aOdJrsqfTFjNw16Zz1LStcE/KHPpYohiXtUNBh48HWCzrzGnMPb3tAqpLYxbDebClEWzHRkaooSUrTaDtKpUE+Niz9ydjhMYB/9WmMRSaSHqRop97THhRish6rvaSIVeCG8Dw1Gf203d5v2rM5Pq2L31ohpqOQuevtsCdqHweekvlb7DBHlBFYWMmAyTV5WcX9Yy2gaObnNSAaaB5uCgjYMkb+WHAdzWeIDuw/HsdR6xCUdCrP1+s/P1Mc5b+uJ9YO6k4JKnzgiSMZPRI9XQbHyF5QkMiqTtQ65nVQfjGwizWN3G2sqLe1jNJVRgpwCHsz2s8sWaHd6dH3wyMVXeiYzvPiIHjL5qpbgaMptpwZ0oVAClmaW8Hz6ZstyfPkHr4Agve9a1IO9CTaOgRf8VkCQOSwBPt3MPaZc5alK6utuS4houjyVcjsWAsz/kQuAlaRHeKDE6eIQ6N6ehf8cPUHZ89InhkuER7mnm5PvUWXUQA2+HmnJQFRdHqxD+Uf7d499vaPVK40Tw2HyT+WPL6qXH0sAZp//p90/G3RFpohVs30Emu+nRkm3B6u1jmroVD2hC37gMdtwz1HgDqq4jesQPICzZOacvlGR2+iLIOERWeMm2yVW7UEE66HPCYt6gxw/iu2qG4rmPIjVhEIyb2xZ5SijelijWOTKZimWyw/aKZRqUg2uXVQfJe9hWWk2ULCVhDC6cZu9shddEk5W6B5loBqukVSnEh7fV4qN/mffTF2gmFvdONyT5YxonWrJh96/BhSjxKUBQdbO6Pa5SUHM0aWBrq+KIKGNGagsQAYMbdiOY/J8LJh87RG1M1qBpHkvyU2DOlZPGfT1xqASvfpLLyhd0HvQ8ItOR0AoJMwRTdoQDVVohdHktLFEQ2lxbuvdQC5zh2lYzZPeorGF7yy+NL7/Ak6FjIwIQ5UaGkKMYzInbYqihFCsSyFIXLn37tWq2eVmL8V5F+5Dc4fAkFBTwBJInMpCp1LYWnUR0JKWmRFnudF2sziDn7Smdus7yl5hhGc31biDbzU0u2pMCycmZ4D1nNXCT/9gry0MpuSt5gbQyhyi99E4BRqKZELFCdhFe47HWXFQDqVt3DyI1IIFuI3PYPnIn/L5QqFk7b/jrxqzcjetlCnMK9MQ6vgsFIWFLSIH2jC1JFY9PAIx+aepIHaZUNhobc3cqLfgt0XfakQnKO8Id6sJNH4OchGv6lkl1fr0imlnLhw0u3sEN8XY+/DMKz3lPye8BT9ajfsBeAcHx0QdR6fXuLhjjMrHSyOKXTQos3hvhFLfCiIz7sCZsQyO6wAPqPDLUvODZh9oylK1Ye2Xgs4JcISmngEmn1mbBGcqToaB9nzjIbeB+8I3Inx8J98FyndpJeVl6vpk16SYgRKxxmlVnbR6yo7kUAM0s2kOtOIQNZ371eXSBlY3R5w/xBZVvbtL5Ghe1yLu8VbaZa6JHigd0owSSI4pdfWQnFpEmmjarn180P8ioBVfofaFZ8OIQGNGtqRQq8JLP6jsiTcEahOwtSkPqOkUo8sCtjtyS+LRM8wAismiTZSozQZ+R29+ETXQOFBjeiW3462YK8GsJC1tQgsI6lB/fRUznMbSYjxxTJX1lLpVKRXk6UCkgRTR8KtQ1d4OtxEX2dh0Ql4LYl93ICcBdp3LIL31iRggd6vvVEnofJmKDYcnEXtajEpdAOGPALOptaSs9ECxAvQYYL3q3sSsFi+sBNMFXZ3YpMtlqWYezV/bLaKdxRCOq8l5V220+O2/hDm+3x7cTEhLLGUwhSOhCpdMSnmdcYbXQOMVjU6zbblxki8J3fmcbOuWRhCPA6F/SpWk5po+dFIbErDIXVCdZpero5SJbxCyVmsNW4svetOOLw930ph0lLpGw8gVe9EojwN8qY4gOVS3n6ISO2CtsDAbEAyrEw1LxIMwt/dPjuypKBamST2dRjj3D0ZNTh630KcRuNbi6xC1DRvvGIhbi7vI7kWAJnGPwYH3a0xxc8x1FYfFlm7Ni1emCK18oRYOLwo+Cy03WoGgT8mzDC7TWGENvD0kejUV/U6xL/nFUybc8ZOuVIfU6we2MMCbwjUp5F9sgf2M1aU9lgxe8cKpk46ES+4TeAqVUBqfUmfm4JH/4qle0yOjs4oZwaRcoSm0tVlENUA3T4coC7s2oS/M4WMgVgtQqySqUXVYIPS7NdKCORABfTxV/L8C6l9qGDcuwZZ8eem4PHkB0KsRh5WfJuChXi5BB/O6+j7hx+TWZ+Y4husIsc1o1BaGMbsjgsVFrVenn++8ok9TOGfTMM//IHOl300V7VXMxz/14W7FwP7+JrdXXJ3BhkCRpGD5xkaNVgc0Ut3ZdpB9GGxmP+Sdx5S2h57XKYm+PTi4hItlJY5yuHYeXnYAklXeUSZlmZ68zYAnmxS2oO7BqF1UzegBH4pWFxjj8dwba7chXTqCd9MBv73lVAhrhx5bGeMVijTzssHa5yQtwnvpzwb652oFgLOSdWL6OO08QdXe8Bo7NzFgRiKnCVgNqP3TsqD8j0Ne3aPcnICaZaskltFd64er+f1Pm2FVq/98C2M0MtfXPFzvhmew4Qbeu+bsJpp3+/VhHODQIoUb2+zSMbOr36WskPsVWGm+zvXEPYY+Gq1dusuxZe1hf7ruwkL+0y9GT4BT/L/3DsP/Qj5nO8JR4P6R8X71oWNmDcNITAECPhJtk78/8MWR5+4hdD+UMRai0kQtBL2pNJwUPG0sj0QJg/O3dcYfbAxhGmJXGiKQSbhWxRF2k5YvD1bnAAdQp0i6QMlHSXti3oK+m/F+Rg/rLeKc6lQIjsiANRGm2iJ0Guuy1EbI9JpNcZQiVKDprn9vGu139AGXi0l5AP5Bj2tD4mKd2Fd9Bp1WjuMoN5c9JoDbuZLiWadNfIOBaVWae1ytb1pQRYseCiNKSQbUMDsBTYtLCLN6cmiQyrKWpPrdQ2GoSsVhMgjWkFXC97iQKJDXKJPlmGBxauSP1k4GGENXMnGgFMiC07nu35jU4n9WUj2AE8NpozvJ5qhk1L3GrPpTWSVSKPgPwX8VEUbvNgfhU0VK2ggZNpd4xJS3u/9cqlNcwYLAzZz+oSH/o6h7gjRMu8a9SKJBPVq4VQEhMzn6GM7Jl4P+OAWV69MowFxAdfnaS5NIYBinYd53Z+rwSFblHDJHPneUlqsOnrViUqpMVTPEGQSnWckBL5k/qejVJVwn+smFnkhbORXXP8SrMrfla/Jdvxm715rwHXFQLFw8dSlsMDpx9BLlPkepR+I1FDtf9tS7XljSDH4Z1F1J63RZGoLC/GYCjWHFQ3tXW3t60MCVg/HaJB34aEKUGlvAoeW3c9nR8a8Hjn/r73rWm8VhsGsgFkBEnbf/z1PaU6LTbGGbUjar7rLRRNX1rLGr13/rx6Avm3JKwcsoxIz9ppvAajqgFMh3E8MDQlZgh5astG+jv5syieIGTHn4bsxxJFmFAswmjuJU6B1c7++uQUwvzLCrVyXdExrVm5oE6rojhBWtKZNPAbRYVluBCPmFJCEP/UbEJU07lpW2UWog63bYgItob9XvG1oGWGF2xtkCeaqumU2ZVlyfL3W3WgwlqO6Dv2WcjAxtTccF+j1V7eyUkgbPj/uteXnxMtl27EJSoIYIqJ+QrhMu2VhQKGAmoyauN4p/h1GKrInEOaBQrFfYwEQKIBNpuGoOtWTkC+FwgkwMQaCdfVYiv6bWABvxA2cyUm8UDgBGilQKHRtHvEb24k8Lig6nRcKJ2DX1ZLz/rToNAGbPVTAn9vx9kKxmBUY0PhgkUxXbgI0BCnhqqHO4bZTAdZDnrAg3orWH0BBGTKcrmTrkvZYlBoFFARZkwevKtFf8I1aWNxhY0dWJ1tSkMg6OE4vYMglfY0eLKgj89Ch/KPpcBwn5NF9jBOwI4V6vECAcaxOlcu8iIKjOKH8DMYJ2GaCG7fARiSseqkcsjoIUFopLWHrNEVPbzzihBb4bLjCi/oYFH4ZiRRFMs/SN4OgguJ6UCQJxXaOR4RashtFBS9EWiXgxia4/QatYMo+NWmcu1Qh6z5aRMTUA/GJMI7qbgeu1tG5dyNy3RUvIg5IgxnW2ASrF166k8Mf16ZTNJzQBQNCRdUd6b/BIbvk50HndquNfE1oW1KB4VniMFdIC+ecc2y8U3MhGwr0GKLlNGtquInUNdH/Tvb8LtGLZDOGLwTJsE7UjuDgELHAjaFs3Cp3CyskaUNjWeFX9kKBiwVuLuRz9K4e7BLQFG4yc7Rk3xVc67QbuYeMb0gceVTZCqKtaigoLFVzSwxnF48jG+cqIj0xcUNxRyvUxMY/gXV843GkfIdOUK18DpDcgDbvkzvoBSZfeKQlabYLLyLH0Og9BHgLx83kDjRczRgCXdsHWpLxwZYRCh/vV+co7YQpG/oWkg2ONXqohMOGv45xIH1Ws3SBOiN015ckWdbAb5IZvHFyJlaOlJYopmSQZMmytJxSJh4bI0NR5g2SjQ3eQheSvYhlprMhRylFj3OCvegKX/CN5lgln2yFHio1DyJrGQvKqnq+Fc/wmaM5pN6mlUO9UaWrpHSFmgx14yqC8ULKLXaFC6GAbWZB6o9tDI5AUBFMR6bEgVjciJ6IxgmzIOdCaLyFWxSlElNX2gsFeKE5xWLyBkxYrxrMp65RorFYNDRHSvGiFsXcksJoMNaSYHkNkcmlmAIKWkPaZh7z7ElI2QYFdnqsYmEIWE8b0vdrEidsKnYBpU8fGmGRxMJoyF6yNoClCGgTw7FVpDdR5vahds01UxqbXMkqVXr3ITLiYIUl8HNDskb6vn/pWg1MlpSn0ApVeScC+dlWIgqSZX6bfTy2qPlh1upJtRBd5MEj+3J2SJv30vYVr6OuBiHvHX3FkMfROgdNDj7NJmkHhRrzmHetWuqepBkVr8PJVnkR0KySbgZhfaBeuUHv+qf7cFBFS8X7jB01RGVEYIx9JVn9KTvivMMCRd94GDsrWA5EXkTtnmmaTEMLaRHSDpPFQAaziRx2ekxEQUzmHUO/NngxNWSd4dpxPtt5reMCCkNebJayPq7PFAtv0OtHmdEhadxy4l1tqbzYEYyBjt2vUK9l4R5kh5YTzluLybx4i7foBWugNHMCvnWQY6NY3yZ+z+XEu47QAZY24C+r+WO9Q1bMVHWUJ5gZIOLR5B1AA50XmwH51shYrJ5HdgA8zKvooGbzjIE3pQxTr230HGn9SmpKU7F5y9ANl/HEt7wA5xwSmMbqTkdGVPH1pv3a9bPZdmqXVbKkS806yedQtW7RB/wO3C5KyT/XpZNN1sU7kMKZdy3/gXcag0ryRdWq8M5kBFqxsqXixjtQ3CxwTJOB3fzKfyWZJy5NxePDW3r8ZKO4R7wzReOQr3aTbtHva3lwD4cKzRkcvWWdkVSVAtBqTvnx5pcDSWL+MpIjDabZ49imC2o0/xEgv+jcYLSp8SGpyAxFZ8yItC2800hkV8NjJlUHrHX5+O7HTprOmNvXzDuV/N5i3dey46Lrm3bKgmUd1wddLsGyW8Rym9qSV/S9k4mKcAWfO0nTKIriOI6iNE3sl6kB2CaHCsbo4OhuKRl97ynEyhucQdVTRIKfTTqc0t73nkgiq1+FE3UmvOeSBiz6dN24597ziYmlfgRFN997DfoGwXmykRgv3suQCOanMSOdA+G9EpXZc5iRzlnpvRqVwXi6zYjG4PUY8WBmgTYAAADZSURBVGBGH5/JiLh/UUY8vElTnRSNJ1XjC++lKRy6E4xG2g2h9/pUBgeLxrtAvLJmbEJQ4pYrEz7EY5Z7P4jEshsnTg7gw/TqFmKXHf7Qu9SUZV3CT+TDp6YETR05YEcS1U2Qez+ZxLI+c7jVNrqSxPVt8Et8y/OPoMLP2vHKF48kuo5t5hfe7yKRX7K2rysaQ5Koqvs2u+TC+61U5n4wtf1cV8v6cDXH/f4xjeKqnvt2Cvy89H47iUfd52N9eDZM93v7Qff7NGTBsvn7US0S3h/90R/90SvSPzbRgMYUdimgAAAAAElFTkSuQmCC", home_btn.size());
  flag_img = loadPixmap("../assets/images/button_flag.png", home_btn.size());
  settings_img = loadPixmap("../assets/images/button_settings.png", settings_btn.size(), Qt::IgnoreAspectRatio);

  connect(this, &Sidebar::valueChanged, [=] { update(); });

  setAttribute(Qt::WA_OpaquePaintEvent);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  setFixedWidth(300);

  QObject::connect(uiState(), &UIState::uiUpdate, this, &Sidebar::updateState);

  pm = std::make_unique<PubMaster, const std::initializer_list<const char *>>({"userFlag"});
}

void Sidebar::mousePressEvent(QMouseEvent *event) {
  if (onroad && home_btn.contains(event->pos())) {
    flag_pressed = true;
    update();
  } else if (settings_btn.contains(event->pos())) {
    settings_pressed = true;
    update();
  }
}

void Sidebar::mouseReleaseEvent(QMouseEvent *event) {
  if (flag_pressed || settings_pressed) {
    flag_pressed = settings_pressed = false;
    update();
  }
  if (home_btn.contains(event->pos())) {
    MessageBuilder msg;
    msg.initEvent().initUserFlag();
    pm->send("userFlag", msg);
  } else if (settings_btn.contains(event->pos())) {
    emit openSettings();
  }
}

void Sidebar::offroadTransition(bool offroad) {
  onroad = !offroad;
  update();
}

void Sidebar::updateState(const UIState &s) {
  if (!isVisible()) return;

  auto &sm = *(s.sm);

  auto deviceState = sm["deviceState"].getDeviceState();
  setProperty("netType", network_type[deviceState.getNetworkType()]);
  int strength = (int)deviceState.getNetworkStrength();
  setProperty("netStrength", strength > 0 ? strength + 1 : 0);

  ItemStatus connectStatus;
  auto last_ping = deviceState.getLastAthenaPingTime();
  if (last_ping == 0) {
    connectStatus = ItemStatus{{tr("CONNECT"), tr("OFFLINE")}, warning_color};
  } else {
    connectStatus = nanos_since_boot() - last_ping < 80e9
                        ? ItemStatus{{tr("CONNECT"), tr("ONLINE")}, good_color}
                        : ItemStatus{{tr("CONNECT"), tr("ERROR")}, danger_color};
  }
  setProperty("connectStatus", QVariant::fromValue(connectStatus));

  ItemStatus tempStatus = {{tr("TEMP"), tr("HIGH")}, danger_color};
  auto ts = deviceState.getThermalStatus();
  if (ts == cereal::DeviceState::ThermalStatus::GREEN) {
    tempStatus = {{tr("TEMP"), tr("GOOD")}, good_color};
  } else if (ts == cereal::DeviceState::ThermalStatus::YELLOW) {
    tempStatus = {{tr("TEMP"), tr("OK")}, warning_color};
  }
  setProperty("tempStatus", QVariant::fromValue(tempStatus));

  ItemStatus pandaStatus = {{tr("VEHICLE"), tr("ONLINE")}, good_color};
  if (s.scene.pandaType == cereal::PandaState::PandaType::UNKNOWN) {
    pandaStatus = {{tr("NO"), tr("PANDA")}, danger_color};
  } else if (s.scene.started && !sm["liveLocationKalman"].getLiveLocationKalman().getGpsOK()) {
    pandaStatus = {{tr("GPS"), tr("SEARCH")}, warning_color};
  } else if (s.scene.started && sm["liveLocationKalman"].getLiveLocationKalman().getGpsOK()) {
    float gpsAccuracy = sm["gpsLocationExternal"].getGpsLocationExternal().getAccuracy();
    pandaStatus = {{tr("GPS"), QString("%1 m").arg(fmin(99, gpsAccuracy), 0, 'f', 2)}, good_color};
  }
  setProperty("pandaStatus", QVariant::fromValue(pandaStatus));
}

void Sidebar::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  p.setPen(Qt::NoPen);
  p.setRenderHint(QPainter::Antialiasing);

  p.fillRect(rect(), QColor(57, 57, 57));

  // buttons
  p.setOpacity(settings_pressed ? 0.65 : 1.0);
  p.drawPixmap(settings_btn.x(), settings_btn.y(), settings_img);
  p.setOpacity(onroad && flag_pressed ? 0.65 : 1.0);
  p.drawPixmap(home_btn.x(), home_btn.y(), onroad ? flag_img : home_img);
  p.setOpacity(1.0);

  // network
  int x = 58;
  const QColor gray(0x54, 0x54, 0x54);
  for (int i = 0; i < 5; ++i) {
    p.setBrush(i < net_strength ? Qt::white : gray);
    p.drawRect(x, 196, 27, 27);
//    p.drawEllipse(x, 196, 27, 27);
    x += 37;
  }

  p.setFont(InterFont(35));
  p.setPen(QColor(0xff, 0xff, 0xff));
  const QRect r = QRect(50, 247, 100, 50);
  p.drawText(r, Qt::AlignCenter, net_type);

  // metrics
  drawMetric(p, temp_status.first, temp_status.second, 338);
  drawMetric(p, panda_status.first, panda_status.second, 496);
  drawMetric(p, connect_status.first, connect_status.second, 654);
}
