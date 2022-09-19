\NeedsTeXFormat{LaTeX2e}
\RequirePackage{iftex}
\RequireLuaTeX

\ProvidesClass{unibsthesis}[2022/09/19 unibs unofficial thesis template by @tetofonta]

\newif\if@appendix\@appendixfalse

\newif\ifunibsthesis@bytefield
\newif\ifunibsthesis@listings
\newif\ifunibsthesis@bibliography

\DeclareOption{appendix}{\@appendixtrue}

\DeclareOption{bytefield}{\unibsthesis@bytefieldtrue}
\DeclareOption{listings}{\unibsthesis@listingstrue}
\DeclareOption{bibliography}{\unibsthesis@bibliographytrue}

\DeclareOption*{\PassOptionsToClass{\CurrentOption}{book}}

\ProcessOptions\relax
\LoadClass[12pt]{book}

%%%% MainPage %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

\RequirePackage{graphicx}
\RequirePackage[
    a4paper,
    margin=3cm,
    bindingoffset=1cm,
    lines=25,
    includeheadfoot
]{geometry}
\RequirePackage[hidelinks]{hyperref}
\RequirePackage{bookmark}
\RequirePackage{setspace}
\RequirePackage{afterpage}
\RequirePackage{subfig}
\RequirePackage[labelfont=bf]{caption}


\makeatletter
    \newcommand*{\@unibslogo}{logo.png}
    \newcommand*{\unibslogo}[1]{\gdef\@unibslogo{#1}}

    \newcommand*{\@department}{INGEGNERIA DELL'INFORMAZIONE}
    \newcommand*{\@degreename}{Ingegneria Informatica}
    \newcommand*{\@supervisor}{}
    \newcommand*{\@cosupervisor}{}
    \newcommand*{\@additionalcosupervisors}{}
    \newcommand*{\@studentno}{}
    \newcommand*{\@gender}{o}
    \newcommand*{\@thesistype}{}
    \newcommand*{\@degreetype}{}

    \newcommand*{\department}[1]{\gdef\@department{#1}}
    \newcommand*{\degreename}[1]{\gdef\@degreename{#1}}
    \newcommand*{\supervisor}[1]{\gdef\@supervisor{#1}}
    \newcommand*{\cosupervisor}[1]{\gdef\@cosupervisor{#1}}
    \newcommand*{\additionalcosupervisors}[1]{\gdef\@additionalcosupervisors{#1}}
    \newcommand*{\studentno}[1]{\gdef\@studentno{#1}}
    \newcommand*{\male}{\gdef\@gender{o}}
    \newcommand*{\female}{\gdef\@gender{a}}
    \newcommand*{\finalrelation}{
        \gdef\@thesistype{Relazione Finale}
        \gdef\@degreetype{Corso di Laurea}
    }
    \newcommand*{\thesis}{
        \gdef\@thesistype{Tesi di Laurea}
        \gdef\@degreetype{Corso di Laurea magistrale}
    }

    \renewcommand*{\maketitle}{%
        \pagenumbering{gobble}
        \newgeometry{left=20mm,right=20mm,top=25mm,bottom=20mm}
        \begin{titlepage}
            \begin{center}
                \begin{figure}[t]
                    \centering
                    \includegraphics[height=30mm]{\@unibslogo}    
                \end{figure}
                \vspace*{5mm}
                {\fontsize{16}{16} \textbf{DIPARTIMENTO DI \\ \@department \\}}
                \vspace*{10mm}
                {\fontsize{17}{17}\selectfont \@degreetype\ in\\ \@degreename \\}
                \vspace*{10mm}
                {\fontsize{20}{20}\selectfont
                    \@thesistype\\
                    \vspace*{3mm}
                    \textbf{\@title}\\
                }
            \end{center}
            \vfill
            \begin{flushleft}
                \fontsize{16}{16}\selectfont 
                \textbf{Relatore}:\\
                \@supervisor \\
                \if\relax\@cosupervisor\else
                    \textbf{Correlatore}:\\
                    \@cosupervisor \\
                    \if\relax\@additionalcosupervisors\else
                        \@additionalcosupervisors
                    \fi
                \fi    
            \end{flushleft}
            \begin{flushright}
                {\fontsize{17}{17}\selectfont 
                    Laureand\@gender: \\
                    \@author \\
                    Matricola n. \@studentno \\
                }
            \end{flushright}
            \vspace*{5mm}
            \begin{center}
                {\fontsize{17}{17}\selectfont Anno Accademico \@date}
            \end{center}
        \end{titlepage}
        \restoregeometry%
        \cleardoublepage%
    }
\makeatother


\hypersetup{
    pdftitle={\relax\@title},
    pdfsubject={\relax\@thesistype},
    pdfauthor={\relax\@author}
}

%%%% Spaziatura %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

\onehalfspacing%
\setstretch{1.5}
\setlength\parskip{0em plus 10pt}

%%%% HeaderFooter %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

\RequirePackage{fancyhdr}

\fancypagestyle{plain}{%
    \fancyhf{}% Clear header and footer
    \fancyfoot[C]{
        \if\relax\thepage\else%
            --- {\thepage} ---
        \fi
    }
    \renewcommand{\headrulewidth}{0pt}
    \renewcommand{\footrulewidth}{0pt}
}
\pagestyle{plain}

%%%%%%%%%%%%%%%%%%%%%%%%%%%CHAPTER%%%%%%%%%%%%%%%%%%%%%%%%%%%
\RequirePackage{titlesec}
\titleformat{\chapter}{\normalfont\fontsize{18}{27}\bfseries}{\thechapter\hspace{0.75em}---}{0.75em}{}
\titlespacing*{\chapter}{0pt}{5.5ex plus 1ex minus .2ex}{5.3ex plus .2ex}

%%%%%%%%%%%%%%%%%%%%%%%%%%%SECTION%%%%%%%%%%%%%%%%%%%%%%%%%%%
\titleformat{\section}{\normalfont\fontsize{16}{24}\bfseries}{\thesection\hspace{0.5em}--}{0.5em}{}
\titlespacing*{\section}{0pt}{2.5ex plus 1ex minus .2ex}{2.3ex plus .2ex}

%%%%%%%%%%%%%%%%%%%%%%%%%%%SUBSECTION%%%%%%%%%%%%%%%%%%%%%%%%%%%
\titleformat{\subsection}{\normalfont\fontsize{13}{19}\bfseries}{\thesubsection\hspace{0.5em}--}{0.5em}{}
\titlespacing*{\subsection}{0pt}{1.5ex plus 1ex minus .2ex}{1.3ex plus .2ex}

%%%%%%%%%%%%%%%%%%%%%%%%%%%SUBSUBSECTION%%%%%%%%%%%%%%%%%%%%%%%%%%%
\titleformat{\subsubsection}{\normalfont\fontsize{12}{18}\itshape\bfseries}{}{0em}{}
\titlespacing*{\subsection}{0pt}{1.5ex plus 1ex minus .2ex}{1.3ex plus .2ex}

%%%%%%%%%%%%%%%%%%%%%%%%%%% Numbering %%%%%%%%%%%%%%%%%%%%%%%%%%%
\setcounter{tocdepth}{3}
\setcounter{secnumdepth}{3}

%%%%%%%%%%%%%%%%%%%%%%%%%%% Footnotes %%%%%%%%%%%%%%%%%%%%%%%%%%%

\RequirePackage{footnote}

\makesavenoteenv{tabular}
\makesavenoteenv{table}
\makesavenoteenv{figure}

%%%%%%%%%%%%%%%%%%%%%%%%%%% Appendix %%%%%%%%%%%%%%%%%%%%%%%%%%%
\RequirePackage[subfigure]{tocloft}
\newlistof{appendices}{apc}{}
\newcommand{\appendices}[1]{\addcontentsline{apc}{appendices}{#1}}
\newcommand{\newappendix}[1]{\clearpage\section{#1}\appendices{#1}}
\newcommand*{\makeappendix}{
    \chapter{Appendice}
    \addtocontents{toc}{\protect\setcounter{tocdepth}{0}}
    Di seguito è riportata la lista dei documenti allegati, i quali
    sono poi inseriti a seguire.
    \vspace*{-25mm}
    \listofappendices
}
\newcommand{\attachment}[5]{
    \newappendix{#1}\label{#2}
    Sorgente: \\ \url{#3}\\
    visitato {#4}

    #5
}

%%%%%%%%%%%%%%%%%%%%%%%%%%% Bytefield %%%%%%%%%%%%%%%%%%%%%%%%%%%

\RequirePackage{bytefield}

%%%%%%%%%%%%%%%%%%%%%%%%%%% Lstlisting %%%%%%%%%%%%%%%%%%%%%%%%%%%



%%%%%%%%%%%%%%%%%%%%%%%%%%% Bibliography %%%%%%%%%%%%%%%%%%%%%%%%%%%


