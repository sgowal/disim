function [T D1 D2] = readPEMSDualData(filename, color)

    % Read file
    A = importdata(filename, '\t', 1);
    D1 = A.data(:,end-3);
    D2 = A.data(:,end-2);
    T = zeros(size(D1,1),1);
    for i = 2:size(A.textdata,1)
        timestr = A.textdata{i,1};
        B = sscanf(timestr,'%d/%d/%d %d:%d');
        T(i-1) = B(5) + B(4)*60;
    end
    for i = 1:length(D1)
        if (A.data(i,end) == 0 && i ~= 1)
            D1(i) = D1(i-1);
            D2(i) = D2(i-1);
        end
    end
    
    % Plot
    if (nargout ~= 0)
        return;
    end
    figure();
    if (nargin == 2)
        figure(Z+1);
        hold on;
    else
        color = 'b';
    end
    xlabel('Occupancy [%]');
    ylabel('Flow [veh/5min]');
    plot(D1,D2,'o', 'MarkerEdgeColor','k', 'MarkerFaceColor',color, 'MarkerSize', 5);
    axis([0 140 0 2500]);
    if (nargin == 2)
        hold off;
    end
end